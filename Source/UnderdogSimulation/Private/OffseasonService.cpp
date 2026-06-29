#include "OffseasonService.h"
#include "AwardsService.h"
#include "DevelopmentService.h"
#include "FreeAgencyService.h"
#include "LeagueGenerator.h"
#include "LeagueService.h"
#include "ManagementService.h"
#include "DeterministicRandom.h"

namespace
{
    const TCHAR* DraftFirstNames[] = { TEXT("Jaylen"), TEXT("Tyrese"), TEXT("Cade"), TEXT("Scottie"),
        TEXT("Franz"), TEXT("Jalen"), TEXT("Evan"), TEXT("Alperen"), TEXT("Josh"), TEXT("Tre"),
        TEXT("Ziaire"), TEXT("Keegan") };
    const TCHAR* DraftLastNames[] = { TEXT("Williams"), TEXT("Johnson"), TEXT("Davis"), TEXT("Brown"),
        TEXT("Wilson"), TEXT("Anderson"), TEXT("Thomas"), TEXT("Jackson"), TEXT("White"), TEXT("Harris"),
        TEXT("Martin"), TEXT("Thompson") };

    FGuid DeterministicGuid(FDeterministicRandom& Random)
    {
        return FGuid(Random.NextUInt32(), Random.NextUInt32(), Random.NextUInt32(), Random.NextUInt32());
    }
}

bool FOffseasonService::StartOffseason(FLeagueState& League, FString& OutError)
{
    if (League.Phase != ESeasonPhase::Complete)
    {
        OutError = TEXT("The season must be complete before starting the offseason.");
        return false;
    }

    League.Offseason = FOffseasonState();
    League.Offseason.CurrentStep = EOffseasonStep::Awards;

    TArray<FSeasonAward> SeasonAwards = FAwardsService::CalculateAwards(League);
    if (League.Playoffs.ChampionTeamId.IsValid())
    {
        SeasonAwards.Add(FAwardsService::CalculateChampionMVP(League));
    }
    League.Awards.Append(SeasonAwards);

    League.Offseason.CurrentStep = EOffseasonStep::Aging;
    return true;
}

bool FOffseasonService::AdvanceOffseason(FLeagueState& League, FString& OutError)
{
    if (League.Phase != ESeasonPhase::Complete)
    {
        OutError = TEXT("The season is not in the offseason phase.");
        return false;
    }

    switch (League.Offseason.CurrentStep)
    {
    case EOffseasonStep::Awards:
        StartOffseason(League, OutError);
        break;

    case EOffseasonStep::Aging:
        ProcessAging(League);
        FDevelopmentService::ApplyBreakoutBust(League);
        League.Offseason.CurrentStep = EOffseasonStep::ContractExpiry;
        break;

    case EOffseasonStep::ContractExpiry:
        ProcessContractExpiry(League);
        League.Offseason.CurrentStep = EOffseasonStep::FreeAgency;
        FFreeAgencyService::BuildFreeAgentPool(League);
        break;

    case EOffseasonStep::FreeAgency:
        FFreeAgencyService::AutoSignFreeAgents(League);
        League.Offseason.CurrentStep = EOffseasonStep::Draft;
        GenerateDraftClass(League);
        break;

    case EOffseasonStep::Draft:
        AutoDraft(League);
        League.Offseason.CurrentStep = EOffseasonStep::Resigning;
        break;

    case EOffseasonStep::Resigning:
        ProcessResigning(League);
        League.Offseason.CurrentStep = EOffseasonStep::Complete;
        break;

    case EOffseasonStep::Complete:
        ResetForNewSeason(League);
        break;

    default:
        OutError = TEXT("Unknown offseason step.");
        return false;
    }
    return true;
}

void FOffseasonService::ProcessAging(FLeagueState& League)
{
    FDeterministicRandom Random(static_cast<uint64>(League.LeagueSeed)
        ^ (static_cast<uint64>(League.SeasonNumber) << 32) ^ 0x41474555LL);

    for (FTeamState& Team : League.Teams)
    {
        for (FPlayerProfile& Player : Team.Players)
        {
            Player.Age++;
            if (Player.Age >= 33)
            {
                const int32 DeclineChance = (Player.Age - 32) * 1500;
                if (Random.ChancePerTenThousand(DeclineChance))
                {
                    int32* Ratings[] = { &Player.Ratings.InsideScoring, &Player.Ratings.OutsideShooting,
                        &Player.Ratings.Playmaking, &Player.Ratings.PerimeterDefense,
                        &Player.Ratings.InteriorDefense, &Player.Ratings.Rebounding,
                        &Player.Ratings.Athleticism, &Player.Ratings.Stamina };
                    const int32 NumDeclines = Random.Range(1, 3);
                    for (int32 D = 0; D < NumDeclines; ++D)
                    {
                        int32* Rating = Ratings[Random.Range(0, 7)];
                        *Rating = FMath::Max(1, *Rating - Random.Range(1, 3));
                    }
                    Player.Ratings.Clamp();
                }
            }
        }
    }
}

void FOffseasonService::ProcessContractExpiry(FLeagueState& League)
{
    League.Offseason.ExpiredContractPlayerIds.Reset();
    for (FTeamState& Team : League.Teams)
    {
        for (FPlayerProfile& Player : Team.Players)
        {
            Player.Contract.YearsRemaining--;
            if (Player.Contract.YearsRemaining <= 0)
            {
                League.Offseason.ExpiredContractPlayerIds.Add(Player.PlayerId);
            }
        }
    }
}

void FOffseasonService::GenerateDraftClass(FLeagueState& League)
{
    FDeterministicRandom Random(static_cast<uint64>(League.LeagueSeed)
        ^ (static_cast<uint64>(League.SeasonNumber) << 16) ^ 0x4452414654ULL);

    League.Offseason.DraftClass.Reset();
    League.Offseason.CurrentDraftPick = 0;

    const TArray<FTeamState> Standings = FLeagueService::GetStandings(League);

    for (int32 Pick = 0; Pick < 12; ++Pick)
    {
        FDraftProspect Prospect;
        Prospect.DraftOrder = Pick;

        FPlayerProfile& Profile = Prospect.Profile;
        Profile.PlayerId = DeterministicGuid(Random);
        Profile.DisplayName = FString::Printf(TEXT("%s %s"),
            DraftFirstNames[Random.Range(0, 11)], DraftLastNames[Random.Range(0, 11)]);
        Profile.Age = Random.Range(19, 22);
        Profile.HeightCm = Random.Range(185, 213);
        Profile.bLeftHanded = Random.ChancePerTenThousand(1200);
        Profile.Position = static_cast<EPlayerPosition>(Pick % 5);
        Profile.Archetype = static_cast<EPlayerArchetype>(Random.Range(0, 7));

        const int32 BaseQuality = 62 - Pick * 2;
        int32* Ratings[] = { &Profile.Ratings.InsideScoring, &Profile.Ratings.OutsideShooting,
            &Profile.Ratings.Playmaking, &Profile.Ratings.PerimeterDefense,
            &Profile.Ratings.InteriorDefense, &Profile.Ratings.Rebounding,
            &Profile.Ratings.Athleticism, &Profile.Ratings.Stamina };
        for (int32* Rating : Ratings) { *Rating = BaseQuality + Random.Range(-8, 8); }
        Profile.Ratings.Potential = Random.Range(FMath::Max(55, BaseQuality), 95);
        Profile.Ratings.WorkEthic = Random.Range(45, 90);
        Profile.Ratings.Clamp();
        Profile.Contract.SalaryMinorUnits = static_cast<int64>(Profile.Ratings.Overall()) * 1500000LL;
        Profile.Contract.YearsRemaining = 3;

        League.Offseason.DraftClass.Add(Prospect);
    }
}

bool FOffseasonService::DraftPlayer(FLeagueState& League, const FGuid& TeamId,
    int32 ProspectIndex, FString& OutError)
{
    if (League.Offseason.CurrentStep != EOffseasonStep::Draft)
    {
        OutError = TEXT("The draft is not currently active.");
        return false;
    }
    if (ProspectIndex < 0 || ProspectIndex >= League.Offseason.DraftClass.Num())
    {
        OutError = TEXT("Invalid prospect selection.");
        return false;
    }
    FDraftProspect& Prospect = League.Offseason.DraftClass[ProspectIndex];
    if (Prospect.bDrafted)
    {
        OutError = TEXT("This prospect has already been drafted.");
        return false;
    }
    FTeamState* Team = League.Teams.FindByPredicate(
        [&TeamId](const FTeamState& T) { return T.TeamId == TeamId; });
    if (!Team)
    {
        OutError = TEXT("Team not found.");
        return false;
    }

    Prospect.bDrafted = true;
    Prospect.DraftedByTeamId = TeamId;
    Team->Players.Add(Prospect.Profile);

    FAthleteState State;
    State.PlayerId = Prospect.Profile.PlayerId;
    State.Morale = 65;
    Team->PlayerStates.Add(State);

    FString RotationError;
    FManagementService::AutoBuildRotation(League, TeamId, RotationError);
    League.Offseason.CurrentDraftPick++;
    return true;
}

void FOffseasonService::AutoDraft(FLeagueState& League)
{
    const TArray<FTeamState> StandingsCopy = League.Teams;
    TArray<FGuid> DraftOrder;
    for (int32 Index = StandingsCopy.Num() - 1; Index >= 0; --Index)
    {
        DraftOrder.Add(StandingsCopy[Index].TeamId);
    }

    for (int32 Pick = 0; Pick < League.Offseason.DraftClass.Num(); ++Pick)
    {
        FDraftProspect& Prospect = League.Offseason.DraftClass[Pick];
        if (Prospect.bDrafted) { continue; }
        const FGuid& TeamId = DraftOrder[Pick % DraftOrder.Num()];
        FString Error;
        DraftPlayer(League, TeamId, Pick, Error);
    }
}

void FOffseasonService::ProcessResigning(FLeagueState& League)
{
    FDeterministicRandom Random(static_cast<uint64>(League.LeagueSeed)
        ^ (static_cast<uint64>(League.SeasonNumber) << 24) ^ 0x5245534947ULL);

    for (FTeamState& Team : League.Teams)
    {
        for (int32 Index = Team.Players.Num() - 1; Index >= 0; --Index)
        {
            FPlayerProfile& Player = Team.Players[Index];
            if (Player.Contract.YearsRemaining > 0) { continue; }

            if (Player.Ratings.Overall() >= 45 && Team.Players.Num() <= 15)
            {
                Player.Contract.YearsRemaining = Random.Range(1, 3);
                Player.Contract.SalaryMinorUnits = static_cast<int64>(Player.Ratings.Overall()) * 2200000LL;
            }
            else if (Team.Players.Num() > 12)
            {
                const FGuid Pid = Player.PlayerId;
                Team.Players.RemoveAt(Index);
                const int32 StateIdx = Team.PlayerStates.IndexOfByPredicate(
                    [&Pid](const FAthleteState& S) { return S.PlayerId == Pid; });
                if (StateIdx != INDEX_NONE) { Team.PlayerStates.RemoveAt(StateIdx); }
            }
            else
            {
                Player.Contract.YearsRemaining = 1;
                Player.Contract.SalaryMinorUnits = static_cast<int64>(Player.Ratings.Overall()) * 1800000LL;
            }
        }
        FString Error;
        FManagementService::AutoBuildRotation(League, Team.TeamId, Error);
    }
}

void FOffseasonService::ResetForNewSeason(FLeagueState& League)
{
    League.SeasonNumber++;
    League.CurrentRound = 0;
    League.Phase = ESeasonPhase::RegularSeason;
    League.TradeHistory.Reset();
    League.SeasonStats.Reset();
    League.ScoutingAssignments.Reset();
    League.ScoutingReports.Reset();
    League.Playoffs = FPlayoffBracket();
    League.Offseason = FOffseasonState();

    for (FTeamState& Team : League.Teams)
    {
        Team.Wins = 0;
        Team.Losses = 0;
        Team.PointsFor = 0;
        Team.PointsAgainst = 0;
        for (FAthleteState& State : Team.PlayerStates)
        {
            State.Fatigue = 0;
            State.Fitness = 100;
            State.Morale = 50;
            State.InjuryGamesRemaining = 0;
            State.RecentForm = 50;
            State.SeasonDevelopment = 0;
        }
    }

    League.Schedule = FLeagueGenerator::GenerateDoubleRoundRobin(League.Teams,
        static_cast<uint64>(League.LeagueSeed) ^ (static_cast<uint64>(League.SeasonNumber) << 40));

    FDevelopmentService::EstablishMentorships(League);
}
