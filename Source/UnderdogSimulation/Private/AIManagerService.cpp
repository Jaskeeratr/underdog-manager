#include "AIManagerService.h"
#include "ManagementService.h"
#include "TradeService.h"
#include "DeterministicRandom.h"
#include "StaffService.h"

namespace
{
    int32 CountPositionPlayers(const FTeamState& Team, EPlayerPosition Pos)
    {
        int32 Count = 0;
        for (const FPlayerProfile& P : Team.Players) { Count += P.Position == Pos ? 1 : 0; }
        return Count;
    }

    int32 AverageTeamOverall(const FTeamState& Team)
    {
        if (Team.Players.Num() == 0) { return 50; }
        int32 Sum = 0;
        for (const FPlayerProfile& P : Team.Players) { Sum += P.Ratings.Overall(); }
        return Sum / Team.Players.Num();
    }

    int32 ActiveScoutCount(const FLeagueState& League, const FGuid& TeamId)
    {
        int32 Count = 0;
        for (const FScoutingAssignment& A : League.ScoutingAssignments)
        {
            Count += (A.RequestedByTeamId == TeamId && !A.bComplete) ? 1 : 0;
        }
        return Count;
    }
}

void FAIManagerService::ProcessRound(FLeagueState& League, const FGuid& PlayerTeamId)
{
    // Trade execution commits an atomic league copy and can therefore replace the
    // Teams array. Iterate stable IDs instead of retaining array references across
    // a management action that may invalidate them.
    TArray<FGuid> ManagedTeamIds;
    ManagedTeamIds.Reserve(League.Teams.Num());
    for (const FTeamState& Team : League.Teams)
    {
        if (Team.TeamId != PlayerTeamId) { ManagedTeamIds.Add(Team.TeamId); }
    }

    for (const FGuid& TeamId : ManagedTeamIds)
    {
        FTeamState* Team = League.Teams.FindByPredicate(
            [&TeamId](const FTeamState& Candidate) { return Candidate.TeamId == TeamId; });
        if (!Team) { continue; }
        AdjustRotation(League, *Team);
        ChooseTraining(*Team);
        AdjustTactics(*Team);
        AssignScouts(League, *Team);
        ConsiderTrades(League, *Team);
    }
    FStaffService::ProcessRound(League, PlayerTeamId);
}

void FAIManagerService::AdjustRotation(FLeagueState& League, FTeamState& Team)
{
    FString Error;
    FManagementService::AutoBuildRotation(League, Team.TeamId, Error);
}

void FAIManagerService::ChooseTraining(FTeamState& Team)
{
    int32 HighFatigueCount = 0;
    int32 TotalFatigue = 0;
    for (const FAthleteState& State : Team.PlayerStates)
    {
        TotalFatigue += State.Fatigue;
        HighFatigueCount += State.Fatigue > 60 ? 1 : 0;
    }
    const int32 AverageFatigue = Team.PlayerStates.Num() > 0
        ? TotalFatigue / Team.PlayerStates.Num() : 0;

    if (HighFatigueCount >= 4 || AverageFatigue > 55)
    {
        Team.TrainingPlan.Focus = ETrainingFocus::Conditioning;
        Team.TrainingPlan.Intensity = ETrainingIntensity::Recovery;
        return;
    }

    const int32 Avg = AverageTeamOverall(Team);
    int32 LowestCategory = 99;
    ETrainingFocus BestFocus = ETrainingFocus::Balanced;
    int32 ShootingAvg = 0, DefenseAvg = 0, PlaymakingAvg = 0;
    for (const FPlayerProfile& P : Team.Players)
    {
        ShootingAvg += (P.Ratings.OutsideShooting + P.Ratings.InsideScoring) / 2;
        DefenseAvg += (P.Ratings.PerimeterDefense + P.Ratings.InteriorDefense) / 2;
        PlaymakingAvg += P.Ratings.Playmaking;
    }
    if (Team.Players.Num() > 0)
    {
        ShootingAvg /= Team.Players.Num();
        DefenseAvg /= Team.Players.Num();
        PlaymakingAvg /= Team.Players.Num();
    }
    if (ShootingAvg <= DefenseAvg && ShootingAvg <= PlaymakingAvg)
    {
        BestFocus = ETrainingFocus::Shooting;
    }
    else if (DefenseAvg <= PlaymakingAvg)
    {
        BestFocus = ETrainingFocus::Defense;
    }
    else
    {
        BestFocus = ETrainingFocus::Playmaking;
    }

    Team.TrainingPlan.Focus = BestFocus;
    Team.TrainingPlan.Intensity = AverageFatigue < 30
        ? ETrainingIntensity::High : ETrainingIntensity::Normal;
}

void FAIManagerService::AssignScouts(FLeagueState& League, FTeamState& Team)
{
    const int32 Active = ActiveScoutCount(League, Team.TeamId);
    if (Active >= 3) { return; }

    int32 SlotsAvailable = 3 - Active;
    for (const FTeamState& Opponent : League.Teams)
    {
        if (Opponent.TeamId == Team.TeamId || SlotsAvailable <= 0) { continue; }
        for (const FPlayerProfile& Player : Opponent.Players)
        {
            if (SlotsAvailable <= 0) { break; }
            if (Player.Ratings.Overall() < AverageTeamOverall(Team)) { continue; }
            FString Error;
            if (FManagementService::AssignScout(League, Team.TeamId, Player.PlayerId, Error))
            {
                SlotsAvailable--;
            }
        }
    }
}

void FAIManagerService::AdjustTactics(FTeamState& Team)
{
    int32 InsideAvg = 0, OutsideAvg = 0, AthleticismAvg = 0, InteriorDefAvg = 0;
    for (const FPlayerProfile& P : Team.Players)
    {
        InsideAvg += P.Ratings.InsideScoring;
        OutsideAvg += P.Ratings.OutsideShooting;
        AthleticismAvg += P.Ratings.Athleticism;
        InteriorDefAvg += P.Ratings.InteriorDefense;
    }
    if (Team.Players.Num() > 0)
    {
        InsideAvg /= Team.Players.Num();
        OutsideAvg /= Team.Players.Num();
        AthleticismAvg /= Team.Players.Num();
        InteriorDefAvg /= Team.Players.Num();
    }

    Team.Tactics.Offense = OutsideAvg > InsideAvg + 5 ? EOffenseStyle::Perimeter
        : InsideAvg > OutsideAvg + 5 ? EOffenseStyle::Inside : EOffenseStyle::Balanced;
    Team.Tactics.Pace = AthleticismAvg >= 65 ? EPaceStyle::Fast
        : AthleticismAvg <= 45 ? EPaceStyle::Slow : EPaceStyle::Balanced;
    Team.Tactics.Defense = InteriorDefAvg >= 60 ? EDefenseStyle::Zone
        : InteriorDefAvg <= 40 ? EDefenseStyle::Switching : EDefenseStyle::Man;
}

void FAIManagerService::ConsiderTrades(FLeagueState& League, FTeamState& Team)
{
    if (League.Phase != ESeasonPhase::RegularSeason
        || League.CurrentRound >= League.TradeDeadlineRound) { return; }

    const uint64 Seed = static_cast<uint64>(League.LeagueSeed) ^ GetTypeHash(Team.TeamId)
        ^ (static_cast<uint64>(League.CurrentRound) << 32) ^ 0x41494D47ULL;
    FDeterministicRandom Random(Seed);
    if (!Random.ChancePerTenThousand(800)) { return; }

    for (int32 Pos = 0; Pos < 5; ++Pos)
    {
        const EPlayerPosition Position = static_cast<EPlayerPosition>(Pos);
        if (CountPositionPlayers(Team, Position) < 4) { continue; }

        const FPlayerProfile* Weakest = nullptr;
        for (const FPlayerProfile& P : Team.Players)
        {
            if (P.Position != Position) { continue; }
            if (!Weakest || P.Ratings.Overall() < Weakest->Ratings.Overall()) { Weakest = &P; }
        }
        if (!Weakest) { continue; }

        for (const FTeamState& Opponent : League.Teams)
        {
            if (Opponent.TeamId == Team.TeamId) { continue; }
            for (const FPlayerProfile& Target : Opponent.Players)
            {
                if (Target.Position == Position) { continue; }
                if (CountPositionPlayers(Team, Target.Position) >= 4) { continue; }
                if (Target.Ratings.Overall() < Weakest->Ratings.Overall() + 5) { continue; }

                FString Error;
                TArray<FGuid> Outgoing = { Weakest->PlayerId };
                TArray<FGuid> Incoming = { Target.PlayerId };
                FTradeService::ProposeTrade(League, Team.TeamId, Outgoing,
                    Opponent.TeamId, Incoming, Error);
                return;
            }
        }
    }
}
