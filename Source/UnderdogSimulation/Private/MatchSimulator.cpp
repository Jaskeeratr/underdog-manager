#include "MatchSimulator.h"
#include "DeterministicRandom.h"

namespace
{
    bool IsAvailable(const FTeamState& Team, const FGuid& PlayerId)
    {
        const FAthleteState* State = Team.PlayerStates.FindByPredicate(
            [&PlayerId](const FAthleteState& Candidate) { return Candidate.PlayerId == PlayerId; });
        return !State || State->InjuryGamesRemaining <= 0;
    }

    TArray<const FPlayerProfile*> AvailableRotation(const FTeamState& Team)
    {
        TArray<const FPlayerProfile*> Result;
        for (const FGuid& PlayerId : Team.Rotation.OrderedPlayers)
        {
            const FPlayerProfile* Player = Team.Players.FindByPredicate(
                [&PlayerId](const FPlayerProfile& Candidate) { return Candidate.PlayerId == PlayerId; });
            if (Player && IsAvailable(Team, PlayerId)) { Result.Add(Player); }
            if (Result.Num() >= FMath::Max(5, Team.Tactics.RotationDepth)) { break; }
        }
        for (const FPlayerProfile& Player : Team.Players)
        {
            if (Result.Num() >= 5) { break; }
            if (IsAvailable(Team, Player.PlayerId) && !Result.Contains(&Player)) { Result.Add(&Player); }
        }
        return Result;
    }
}

FPlayerBoxScore& FPossessionMatchSimulator::FindBox(TArray<FPlayerBoxScore>& BoxScore, const FGuid& PlayerId)
{
    for (FPlayerBoxScore& Box : BoxScore) { if (Box.PlayerId == PlayerId) { return Box; } }
    FPlayerBoxScore NewBox; NewBox.PlayerId = PlayerId;
    return BoxScore.Add_GetRef(NewBox);
}

const FPlayerProfile& FPossessionMatchSimulator::SelectShooter(const FTeamState& Team, FDeterministicRandom& Random)
{
    const TArray<const FPlayerProfile*> Rotation = AvailableRotation(Team);
    check(Rotation.Num() >= 5);
    int32 TotalWeight = 0;
    for (const FPlayerProfile* Player : Rotation)
    {
        int32 Weight = Player->Ratings.InsideScoring + Player->Ratings.OutsideShooting;
        if (Player->PlayerId == Team.Tactics.PrimaryOption) { Weight += 80; }
        if (Player->PlayerId == Team.Tactics.SecondaryOption) { Weight += 40; }
        TotalWeight += FMath::Max(1, Weight);
    }
    int32 Roll = Random.Range(1, TotalWeight);
    for (const FPlayerProfile* Player : Rotation)
    {
        int32 Weight = Player->Ratings.InsideScoring + Player->Ratings.OutsideShooting;
        if (Player->PlayerId == Team.Tactics.PrimaryOption) { Weight += 80; }
        if (Player->PlayerId == Team.Tactics.SecondaryOption) { Weight += 40; }
        Roll -= FMath::Max(1, Weight);
        if (Roll <= 0) { return *Player; }
    }
    return *Rotation[0];
}

const FPlayerProfile& FPossessionMatchSimulator::SelectSupportingPlayer(
    const FTeamState& Team, const FGuid& Excluded, FDeterministicRandom& Random)
{
    const TArray<const FPlayerProfile*> Rotation = AvailableRotation(Team);
    check(Rotation.Num() >= 5);
    int32 Index = Random.Range(0, Rotation.Num() - 1);
    for (int32 Attempt = 0; Attempt < Rotation.Num(); ++Attempt)
    {
        const FPlayerProfile& Candidate = *Rotation[(Index + Attempt) % Rotation.Num()];
        if (Candidate.PlayerId != Excluded) { return Candidate; }
    }
    return *Rotation[0];
}

FMatchResult FPossessionMatchSimulator::Simulate(const FMatchSnapshot& Snapshot) const
{
    check(Snapshot.HomeTeam.Players.Num() >= 5 && Snapshot.AwayTeam.Players.Num() >= 5);
    FDeterministicRandom Random(static_cast<uint64>(Snapshot.Seed) ^ static_cast<uint64>(Snapshot.SimulationVersion));
    FMatchResult Result;
    Result.GameId = Snapshot.GameId;
    Result.Seed = Snapshot.Seed;
    Result.SimulationVersion = Snapshot.SimulationVersion;

    for (const FPlayerProfile& Player : Snapshot.HomeTeam.Players)
    {
        FPlayerBoxScore Box; Box.PlayerId = Player.PlayerId; Result.HomeBoxScore.Add(Box);
    }
    for (const FPlayerProfile& Player : Snapshot.AwayTeam.Players)
    {
        FPlayerBoxScore Box; Box.PlayerId = Player.PlayerId; Result.AwayBoxScore.Add(Box);
    }

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
            TArray<FPlayerBoxScore>& OffensiveBoxes = bHomePossession ? Result.HomeBoxScore : Result.AwayBoxScore;
            TArray<FPlayerBoxScore>& DefensiveBoxes = bHomePossession ? Result.AwayBoxScore : Result.HomeBoxScore;
            const FPlayerProfile& Shooter = SelectShooter(Offense, Random);
            const FPlayerProfile& Defender = SelectSupportingPlayer(Defense, FGuid(), Random);
            FPlayerBoxScore& Box = FindBox(OffensiveBoxes, Shooter.PlayerId);
            Clock = FMath::Max(0, Clock - Random.Range(8, 24));

            const int32 TurnoverChance = FMath::Clamp(1450 - Shooter.Ratings.Playmaking * 7 + Defender.Ratings.PerimeterDefense * 3, 700, 2300);
            if (Random.ChancePerTenThousand(TurnoverChance))
            {
                Box.Turnovers++;
                FGuid StealerId;
                if (Random.ChancePerTenThousand(6200))
                {
                    FPlayerBoxScore& DefenderBox = FindBox(DefensiveBoxes, Defender.PlayerId);
                    DefenderBox.Steals++;
                    StealerId = Defender.PlayerId;
                    Emit(EMatchEventType::Steal, Period, Clock, Defense.TeamId, Defender.PlayerId, Shooter.PlayerId);
                }
                Emit(EMatchEventType::Turnover, Period, Clock, Offense.TeamId, Shooter.PlayerId, StealerId);
                bHomePossession = !bHomePossession;
                continue;
            }

            const bool bThree = Offense.Tactics.Offense == EOffenseStyle::Perimeter
                ? Random.ChancePerTenThousand(5200) : Offense.Tactics.Offense == EOffenseStyle::Inside
                ? Random.ChancePerTenThousand(2600) : Random.ChancePerTenThousand(3900);
            const int32 Attack = bThree ? Shooter.Ratings.OutsideShooting : Shooter.Ratings.InsideScoring;
            const int32 DefenseRating = bThree ? Defender.Ratings.PerimeterDefense : Defender.Ratings.InteriorDefense;
            const int32 MakeChance = FMath::Clamp((bThree ? 3000 : 4500) + (Attack - DefenseRating) * 45 + (bHomePossession ? 180 : 0), 1800, 7200);

            if (Random.ChancePerTenThousand(bThree ? 450 : 800))
            {
                FPlayerBoxScore& DefenderBox = FindBox(DefensiveBoxes, Defender.PlayerId);
                DefenderBox.Fouls++;
                Emit(EMatchEventType::Foul, Period, Clock, Defense.TeamId, Defender.PlayerId, Shooter.PlayerId);
                const int32 Attempts = bThree ? 3 : 2;
                for (int32 Attempt = 0; Attempt < Attempts; ++Attempt)
                {
                    Box.FreeThrowsAttempted++;
                    if (Random.ChancePerTenThousand(FMath::Clamp(5200 + Shooter.Ratings.OutsideShooting * 35, 5800, 9300)))
                    {
                        Box.FreeThrowsMade++; Box.Points++;
                        Result.HomeScore += bHomePossession ? 1 : 0; Result.AwayScore += bHomePossession ? 0 : 1;
                        Emit(EMatchEventType::FreeThrowMade, Period, Clock, Offense.TeamId, Shooter.PlayerId, Defender.PlayerId);
                    }
                    else
                    {
                        Emit(EMatchEventType::FreeThrowMissed, Period, Clock, Offense.TeamId, Shooter.PlayerId, Defender.PlayerId);
                    }
                }
                bHomePossession = !bHomePossession;
                continue;
            }

            Box.FieldGoalsAttempted++;
            const bool bBlocked = Random.ChancePerTenThousand(FMath::Clamp(250 + Defender.Ratings.InteriorDefense * (bThree ? 2 : 6), 300, 1200));
            if (!bBlocked && Random.ChancePerTenThousand(MakeChance))
            {
                Box.FieldGoalsMade++;
                if (bThree) { Box.ThreePointersMade++; Box.Points += 3; Result.HomeScore += bHomePossession ? 3 : 0; Result.AwayScore += bHomePossession ? 0 : 3; }
                else { Box.Points += 2; Result.HomeScore += bHomePossession ? 2 : 0; Result.AwayScore += bHomePossession ? 0 : 2; }
                FGuid AssisterId;
                if (Random.ChancePerTenThousand(6100))
                {
                    const FPlayerProfile& Assister = SelectSupportingPlayer(Offense, Shooter.PlayerId, Random);
                    FindBox(OffensiveBoxes, Assister.PlayerId).Assists++;
                    AssisterId = Assister.PlayerId;
                }
                Emit(bThree ? EMatchEventType::ThreePointMade : EMatchEventType::TwoPointMade,
                    Period, Clock, Offense.TeamId, Shooter.PlayerId, AssisterId);
                bHomePossession = !bHomePossession;
            }
            else
            {
                if (bBlocked)
                {
                    FindBox(DefensiveBoxes, Defender.PlayerId).Blocks++;
                    Emit(EMatchEventType::Block, Period, Clock, Defense.TeamId, Defender.PlayerId, Shooter.PlayerId);
                }
                Emit(bThree ? EMatchEventType::ThreePointMissed : EMatchEventType::TwoPointMissed,
                    Period, Clock, Offense.TeamId, Shooter.PlayerId, FGuid());
                const int32 OffensiveReboundChance = Offense.Tactics.Rebounding == EReboundPriority::CrashBoards ? 3300 : 2600;
                if (Random.ChancePerTenThousand(OffensiveReboundChance))
                {
                    const FPlayerProfile& Rebounder = SelectSupportingPlayer(Offense, Shooter.PlayerId, Random);
                    FPlayerBoxScore& ReboundBox = FindBox(OffensiveBoxes, Rebounder.PlayerId);
                    ReboundBox.Rebounds++; ReboundBox.OffensiveRebounds++;
                    Emit(EMatchEventType::OffensiveRebound, Period, Clock, Offense.TeamId, Rebounder.PlayerId, FGuid());
                }
                else
                {
                    const FPlayerProfile& Rebounder = SelectSupportingPlayer(Defense, FGuid(), Random);
                    FindBox(DefensiveBoxes, Rebounder.PlayerId).Rebounds++;
                    Emit(EMatchEventType::DefensiveRebound, Period, Clock, Defense.TeamId, Rebounder.PlayerId, FGuid());
                    bHomePossession = !bHomePossession;
                }
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
