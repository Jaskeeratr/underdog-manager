#include "MatchSimulator.h"
#include "DeterministicRandom.h"

FPlayerBoxScore& FPossessionMatchSimulator::FindBox(TArray<FPlayerBoxScore>& BoxScore, const FGuid& PlayerId)
{
    for (FPlayerBoxScore& Box : BoxScore) { if (Box.PlayerId == PlayerId) { return Box; } }
    FPlayerBoxScore NewBox; NewBox.PlayerId = PlayerId;
    return BoxScore.Add_GetRef(NewBox);
}

const FPlayerProfile& FPossessionMatchSimulator::SelectShooter(const FTeamState& Team, FDeterministicRandom& Random)
{
    const int32 Available = FMath::Min(Team.Players.Num(), FMath::Max(5, Team.Tactics.RotationDepth));
    int32 TotalWeight = 0;
    for (int32 Index = 0; Index < Available; ++Index)
    {
        const FPlayerProfile& Player = Team.Players[Index];
        int32 Weight = Player.Ratings.InsideScoring + Player.Ratings.OutsideShooting;
        if (Player.PlayerId == Team.Tactics.PrimaryOption) { Weight += 80; }
        if (Player.PlayerId == Team.Tactics.SecondaryOption) { Weight += 40; }
        TotalWeight += FMath::Max(1, Weight);
    }
    int32 Roll = Random.Range(1, TotalWeight);
    for (int32 Index = 0; Index < Available; ++Index)
    {
        const FPlayerProfile& Player = Team.Players[Index];
        int32 Weight = Player.Ratings.InsideScoring + Player.Ratings.OutsideShooting;
        if (Player.PlayerId == Team.Tactics.PrimaryOption) { Weight += 80; }
        if (Player.PlayerId == Team.Tactics.SecondaryOption) { Weight += 40; }
        Roll -= FMath::Max(1, Weight);
        if (Roll <= 0) { return Player; }
    }
    return Team.Players[0];
}

FMatchResult FPossessionMatchSimulator::Simulate(const FMatchSnapshot& Snapshot) const
{
    check(Snapshot.HomeTeam.Players.Num() >= 5 && Snapshot.AwayTeam.Players.Num() >= 5);
    FDeterministicRandom Random(static_cast<uint64>(Snapshot.Seed) ^ static_cast<uint64>(Snapshot.SimulationVersion));
    FMatchResult Result;
    Result.GameId = Snapshot.GameId;
    Result.Seed = Snapshot.Seed;
    Result.SimulationVersion = Snapshot.SimulationVersion;

    int32 Sequence = 0;
    auto Emit = [&Result, &Sequence](EMatchEventType Type, int32 Period, int32 Clock, const FGuid& Team,
        const FGuid& Primary, const FGuid& Secondary)
    {
        FMatchEvent Event;
        Event.Sequence = Sequence++; Event.Period = Period; Event.ClockSeconds = Clock; Event.Type = Type;
        Event.TeamId = Team; Event.PrimaryPlayerId = Primary; Event.SecondaryPlayerId = Secondary;
        Event.HomeScore = Result.HomeScore; Event.AwayScore = Result.AwayScore;
        Result.Events.Add(Event);
    };
    Emit(EMatchEventType::GameStarted, 1, 720, Snapshot.HomeTeam.TeamId, FGuid(), FGuid());

    int32 Period = 1;
    while (Period <= Result.PeriodsPlayed)
    {
        int32 Clock = Period <= 4 ? 720 : 300;
        bool bHomePossession = Random.ChancePerTenThousand(5000);
        while (Clock > 0)
        {
            const FTeamState& Offense = bHomePossession ? Snapshot.HomeTeam : Snapshot.AwayTeam;
            const FTeamState& Defense = bHomePossession ? Snapshot.AwayTeam : Snapshot.HomeTeam;
            TArray<FPlayerBoxScore>& Boxes = bHomePossession ? Result.HomeBoxScore : Result.AwayBoxScore;
            const FPlayerProfile& Shooter = SelectShooter(Offense, Random);
            FPlayerBoxScore& Box = FindBox(Boxes, Shooter.PlayerId);
            Clock = FMath::Max(0, Clock - Random.Range(8, 24));

            const int32 TurnoverChance = FMath::Clamp(1450 - Shooter.Ratings.Playmaking * 7 + Defense.Players[0].Ratings.PerimeterDefense * 3, 700, 2300);
            if (Random.ChancePerTenThousand(TurnoverChance))
            {
                Box.Turnovers++;
                Emit(EMatchEventType::Turnover, Period, Clock, Offense.TeamId, Shooter.PlayerId, FGuid());
                bHomePossession = !bHomePossession;
                continue;
            }

            const bool bThree = Offense.Tactics.Offense == EOffenseStyle::Perimeter
                ? Random.ChancePerTenThousand(5200) : Offense.Tactics.Offense == EOffenseStyle::Inside
                ? Random.ChancePerTenThousand(2600) : Random.ChancePerTenThousand(3900);
            const int32 Attack = bThree ? Shooter.Ratings.OutsideShooting : Shooter.Ratings.InsideScoring;
            const int32 DefenseRating = bThree ? Defense.Players[0].Ratings.PerimeterDefense : Defense.Players[4].Ratings.InteriorDefense;
            const int32 MakeChance = FMath::Clamp((bThree ? 3000 : 4500) + (Attack - DefenseRating) * 45 + (bHomePossession ? 180 : 0), 1800, 7200);
            Box.FieldGoalsAttempted++;
            if (Random.ChancePerTenThousand(MakeChance))
            {
                Box.FieldGoalsMade++;
                if (bThree) { Box.ThreePointersMade++; Box.Points += 3; Result.HomeScore += bHomePossession ? 3 : 0; Result.AwayScore += bHomePossession ? 0 : 3; }
                else { Box.Points += 2; Result.HomeScore += bHomePossession ? 2 : 0; Result.AwayScore += bHomePossession ? 0 : 2; }
                Emit(bThree ? EMatchEventType::ThreePointMade : EMatchEventType::TwoPointMade,
                    Period, Clock, Offense.TeamId, Shooter.PlayerId, FGuid());
                bHomePossession = !bHomePossession;
            }
            else
            {
                Emit(bThree ? EMatchEventType::ThreePointMissed : EMatchEventType::TwoPointMissed,
                    Period, Clock, Offense.TeamId, Shooter.PlayerId, FGuid());
                const int32 OffensiveReboundChance = Offense.Tactics.Rebounding == EReboundPriority::CrashBoards ? 3300 : 2600;
                if (!Random.ChancePerTenThousand(OffensiveReboundChance)) { bHomePossession = !bHomePossession; }
            }
        }
        Emit(EMatchEventType::PeriodEnded, Period, 0, FGuid(), FGuid(), FGuid());
        if (Period >= 4 && Result.HomeScore == Result.AwayScore)
        {
            Result.PeriodsPlayed++;
            Emit(EMatchEventType::OvertimeStarted, Period + 1, 300, FGuid(), FGuid(), FGuid());
        }
        Period++;
    }
    Emit(EMatchEventType::GameEnded, Result.PeriodsPlayed, 0, FGuid(), FGuid(), FGuid());
    return Result;
}
