#include "ManagementService.h"
#include "DeterministicRandom.h"

namespace
{
    FTeamState* FindTeam(FLeagueState& League, const FGuid& TeamId)
    {
        return League.Teams.FindByPredicate([&TeamId](const FTeamState& Team) { return Team.TeamId == TeamId; });
    }

    FPlayerProfile* FindPlayer(FLeagueState& League, const FGuid& PlayerId, FGuid* OutTeamId = nullptr)
    {
        for (FTeamState& Team : League.Teams)
        {
            if (FPlayerProfile* Player = Team.Players.FindByPredicate(
                [&PlayerId](const FPlayerProfile& Candidate) { return Candidate.PlayerId == PlayerId; }))
            {
                if (OutTeamId) { *OutTeamId = Team.TeamId; }
                return Player;
            }
        }
        return nullptr;
    }

    FGuid DeterministicGuid(FDeterministicRandom& Random)
    {
        return FGuid(Random.NextUInt32(), Random.NextUInt32(), Random.NextUInt32(), Random.NextUInt32());
    }

    int32* RatingForFocus(FPlayerRatings& Ratings, ETrainingFocus Focus, FDeterministicRandom& Random)
    {
        switch (Focus)
        {
        case ETrainingFocus::Shooting: return Random.ChancePerTenThousand(5500)
            ? &Ratings.OutsideShooting : &Ratings.InsideScoring;
        case ETrainingFocus::Playmaking: return &Ratings.Playmaking;
        case ETrainingFocus::Defense: return Random.ChancePerTenThousand(5000)
            ? &Ratings.PerimeterDefense : &Ratings.InteriorDefense;
        case ETrainingFocus::Rebounding: return &Ratings.Rebounding;
        case ETrainingFocus::Conditioning: return &Ratings.Stamina;
        default:
        {
            int32* Options[] = { &Ratings.InsideScoring, &Ratings.OutsideShooting, &Ratings.Playmaking,
                &Ratings.PerimeterDefense, &Ratings.InteriorDefense, &Ratings.Rebounding,
                &Ratings.Athleticism, &Ratings.Stamina };
            return Options[Random.Range(0, UE_ARRAY_COUNT(Options) - 1)];
        }
        }
    }
}

bool FManagementService::AutoBuildRotation(FLeagueState& League, const FGuid& TeamId, FString& OutError)
{
    FTeamState* Team = FindTeam(League, TeamId);
    if (!Team) { OutError = TEXT("The selected team does not exist."); return false; }
    TArray<const FPlayerProfile*> Ranked;
    for (const FPlayerProfile& Player : Team->Players) { Ranked.Add(&Player); }
    Ranked.Sort([](const FPlayerProfile& A, const FPlayerProfile& B)
    {
        return A.Ratings.Overall() > B.Ratings.Overall();
    });
    Team->Rotation.OrderedPlayers.Reset();
    Team->Rotation.TargetMinutes.Reset();
    const int32 Minutes[] = { 36, 34, 32, 30, 28, 24, 20, 18, 10, 8 };
    for (int32 Index = 0; Index < Ranked.Num(); ++Index)
    {
        Team->Rotation.OrderedPlayers.Add(Ranked[Index]->PlayerId);
        Team->Rotation.TargetMinutes.Add(Ranked[Index]->PlayerId,
            Index < UE_ARRAY_COUNT(Minutes) ? Minutes[Index] : 0);
    }
    return Team->Rotation.IsValid(OutError);
}

bool FManagementService::SetTrainingPlan(FLeagueState& League, const FGuid& TeamId,
    ETrainingFocus Focus, ETrainingIntensity Intensity, FString& OutError)
{
    FTeamState* Team = FindTeam(League, TeamId);
    if (!Team) { OutError = TEXT("The selected team does not exist."); return false; }
    Team->TrainingPlan.Focus = Focus;
    Team->TrainingPlan.Intensity = Intensity;
    return true;
}

bool FManagementService::AssignScout(FLeagueState& League, const FGuid& RequestingTeamId,
    const FGuid& PlayerId, FString& OutError)
{
    if (!FindTeam(League, RequestingTeamId))
    {
        OutError = TEXT("The requesting team does not exist.");
        return false;
    }
    FGuid PlayerTeamId;
    if (!FindPlayer(League, PlayerId, &PlayerTeamId))
    {
        OutError = TEXT("The selected athlete does not exist.");
        return false;
    }
    if (PlayerTeamId == RequestingTeamId)
    {
        OutError = TEXT("Owned athletes do not require scouting assignments.");
        return false;
    }
    int32 Active = 0;
    for (const FScoutingAssignment& Assignment : League.ScoutingAssignments)
    {
        if (Assignment.RequestedByTeamId == RequestingTeamId && !Assignment.bComplete) { Active++; }
        if (Assignment.RequestedByTeamId == RequestingTeamId && Assignment.PlayerId == PlayerId)
        {
            OutError = TEXT("This athlete already has a scouting assignment or report.");
            return false;
        }
    }
    if (Active >= 3)
    {
        OutError = TEXT("All three scouting slots are currently occupied.");
        return false;
    }
    for (const FScoutingReport& Report : League.ScoutingReports)
    {
        if (Report.RequestedByTeamId == RequestingTeamId && Report.PlayerId == PlayerId)
        {
            OutError = TEXT("A completed report already exists for this athlete.");
            return false;
        }
    }
    const uint64 Seed = static_cast<uint64>(League.LeagueSeed) ^ GetTypeHash(PlayerId)
        ^ (static_cast<uint64>(League.CurrentRound) << 32);
    FDeterministicRandom Random(Seed);
    FScoutingAssignment Assignment;
    Assignment.AssignmentId = DeterministicGuid(Random);
    Assignment.RequestedByTeamId = RequestingTeamId;
    Assignment.PlayerId = PlayerId;
    Assignment.StartedRound = League.CurrentRound;
    Assignment.CompletionRound = League.CurrentRound + Random.Range(2, 3);
    League.ScoutingAssignments.Add(Assignment);
    return true;
}

void FManagementService::ProcessTraining(FLeagueState& League)
{
    for (FTeamState& Team : League.Teams)
    {
        const int32 IntensityLoad = Team.TrainingPlan.Intensity == ETrainingIntensity::Recovery ? -10
            : Team.TrainingPlan.Intensity == ETrainingIntensity::High ? 7 : 2;
        for (FAthleteState& State : Team.PlayerStates)
        {
            State.Fatigue = FMath::Clamp(State.Fatigue + IntensityLoad, 0, 100);
            State.Fitness = 100 - State.Fatigue;
            if (State.InjuryGamesRemaining > 0 || Team.TrainingPlan.Intensity == ETrainingIntensity::Recovery
                || State.SeasonDevelopment >= 4) { continue; }
            FPlayerProfile* Player = Team.Players.FindByPredicate(
                [&State](const FPlayerProfile& Candidate) { return Candidate.PlayerId == State.PlayerId; });
            if (!Player || Player->Ratings.Overall() >= Player->Ratings.Potential) { continue; }
            const uint64 Seed = static_cast<uint64>(League.LeagueSeed) ^ GetTypeHash(Player->PlayerId)
                ^ (static_cast<uint64>(League.CurrentRound) << 24) ^ 0x545241494EULL;
            FDeterministicRandom Random(Seed);
            const int32 Chance = Team.TrainingPlan.Intensity == ETrainingIntensity::High
                ? 1400 + Player->Ratings.WorkEthic * 10 : 650 + Player->Ratings.WorkEthic * 7;
            if (Random.ChancePerTenThousand(Chance))
            {
                int32* Rating = RatingForFocus(Player->Ratings, Team.TrainingPlan.Focus, Random);
                *Rating = FMath::Clamp(*Rating + 1, 1, 99);
                State.SeasonDevelopment++;
            }
        }
    }
}

void FManagementService::ProcessScouting(FLeagueState& League)
{
    for (FScoutingAssignment& Assignment : League.ScoutingAssignments)
    {
        if (Assignment.bComplete || Assignment.CompletionRound > League.CurrentRound) { continue; }
        FPlayerProfile* Player = FindPlayer(League, Assignment.PlayerId);
        if (!Player) { continue; }
        FDeterministicRandom Random(static_cast<uint64>(League.LeagueSeed) ^ GetTypeHash(Player->PlayerId)
            ^ 0x53434F5554ULL);
        const int32 Overall = Player->Ratings.Overall();
        const int32 OverallError = Random.Range(3, 6);
        const int32 PotentialError = Random.Range(4, 8);
        FScoutingReport Report;
        Report.AssignmentId = Assignment.AssignmentId;
        Report.RequestedByTeamId = Assignment.RequestedByTeamId;
        Report.PlayerId = Assignment.PlayerId;
        Report.OverallMin = FMath::Clamp(Overall - OverallError, 1, 99);
        Report.OverallMax = FMath::Clamp(Overall + OverallError, 1, 99);
        Report.PotentialMin = FMath::Clamp(Player->Ratings.Potential - PotentialError, 1, 99);
        Report.PotentialMax = FMath::Clamp(Player->Ratings.Potential + PotentialError, 1, 99);
        Report.CompletedRound = League.CurrentRound;
        League.ScoutingReports.Add(Report);
        Assignment.bComplete = true;
    }
}

void FManagementService::ProcessRound(FLeagueState& League)
{
    ProcessTraining(League);
    ProcessScouting(League);
}
