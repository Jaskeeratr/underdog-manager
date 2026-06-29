#pragma once

#include "CoreMinimal.h"
#include "UnderdogCoreTypes.generated.h"

UENUM(BlueprintType)
enum class EPlayerPosition : uint8 { PG, SG, SF, PF, C };

UENUM(BlueprintType)
enum class EPlayerArchetype : uint8
{
    FloorGeneral, Shooter, Slasher, TwoWayWing, StretchBig, RimProtector, Rebounder, AllRounder
};

UENUM(BlueprintType)
enum class EPaceStyle : uint8 { Slow, Balanced, Fast };

UENUM(BlueprintType)
enum class EOffenseStyle : uint8 { Inside, Balanced, Perimeter };

UENUM(BlueprintType)
enum class EDefenseStyle : uint8 { Man, Switching, Zone };

UENUM(BlueprintType)
enum class EReboundPriority : uint8 { Transition, Balanced, CrashBoards };

UENUM(BlueprintType)
enum class ETrainingFocus : uint8 { Balanced, Shooting, Playmaking, Defense, Rebounding, Conditioning };

UENUM(BlueprintType)
enum class ETrainingIntensity : uint8 { Recovery, Normal, High };

UENUM(BlueprintType)
enum class EMatchEventType : uint8
{
    GameStarted, TwoPointMade, TwoPointMissed, ThreePointMade, ThreePointMissed,
    FreeThrowMade, FreeThrowMissed, OffensiveRebound, DefensiveRebound, Turnover,
    Steal, Block, Foul, Substitution, Timeout, PeriodEnded, OvertimeStarted, GameEnded
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FPlayerRatings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 InsideScoring = 50;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 OutsideShooting = 50;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Playmaking = 50;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 PerimeterDefense = 50;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 InteriorDefense = 50;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Rebounding = 50;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Athleticism = 50;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Stamina = 50;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Potential = 50;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 WorkEthic = 50;

    void Clamp();
    int32 Overall() const;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FContract
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 SalaryMinorUnits = 100000000;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 YearsRemaining = 1;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FPlayerProfile
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid PlayerId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Age = 24;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 HeightCm = 198;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bLeftHanded = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EPlayerPosition Position = EPlayerPosition::PG;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EPlayerArchetype Archetype = EPlayerArchetype::AllRounder;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FPlayerRatings Ratings;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FContract Contract;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FAthleteState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid PlayerId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Fitness = 100;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Fatigue = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Morale = 50;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 InjuryGamesRemaining = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 RecentForm = 50;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 SeasonDevelopment = 0;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FTrainingPlan
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) ETrainingFocus Focus = ETrainingFocus::Balanced;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) ETrainingIntensity Intensity = ETrainingIntensity::Normal;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FTeamTactics
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EPaceStyle Pace = EPaceStyle::Balanced;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EOffenseStyle Offense = EOffenseStyle::Balanced;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EDefenseStyle Defense = EDefenseStyle::Man;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EReboundPriority Rebounding = EReboundPriority::Balanced;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid PrimaryOption;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid SecondaryOption;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 RotationDepth = 9;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 AutoTimeoutDeficit = 10;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FRotationPlan
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FGuid> OrderedPlayers;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FGuid, int32> TargetMinutes;
    bool IsValid(FString& OutError) const;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FTeamState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid TeamId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString City;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Nickname;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FPlayerProfile> Players;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FAthleteState> PlayerStates;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FRotationPlan Rotation;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FTeamTactics Tactics;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FTrainingPlan TrainingPlan;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 OperatingBalanceMinorUnits = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 SalaryCapMinorUnits = 14000000000;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Wins = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Losses = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 PointsFor = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 PointsAgainst = 0;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FScheduledGame
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid GameId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid HomeTeamId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid AwayTeamId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Round = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bComplete = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 HomeScore = -1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 AwayScore = -1;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FScoutingAssignment
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid AssignmentId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid RequestedByTeamId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid PlayerId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 StartedRound = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 CompletionRound = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bComplete = false;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FScoutingReport
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid AssignmentId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid RequestedByTeamId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid PlayerId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 OverallMin = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 OverallMax = 99;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 PotentialMin = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 PotentialMax = 99;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 CompletedRound = 0;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FLeagueState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 SchemaVersion = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 SimulationVersion = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 LeagueSeed = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 CurrentRound = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FTeamState> Teams;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FScheduledGame> Schedule;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FScoutingAssignment> ScoutingAssignments;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FScoutingReport> ScoutingReports;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FMatchSnapshot
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid GameId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 SimulationVersion = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 Seed = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FTeamState HomeTeam;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FTeamState AwayTeam;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FMatchEvent
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Sequence = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Period = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ClockSeconds = 720;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EMatchEventType Type = EMatchEventType::GameStarted;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid TeamId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid PrimaryPlayerId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid SecondaryPlayerId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 HomeScore = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 AwayScore = 0;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FPlayerBoxScore
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid PlayerId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Points = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Rebounds = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 OffensiveRebounds = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Assists = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Steals = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Blocks = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Turnovers = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Fouls = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 FieldGoalsMade = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 FieldGoalsAttempted = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ThreePointersMade = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 FreeThrowsMade = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 FreeThrowsAttempted = 0;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FMatchResult
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid GameId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 Seed = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 SimulationVersion = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 HomeScore = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 AwayScore = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 PeriodsPlayed = 4;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FMatchEvent> Events;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FPlayerBoxScore> HomeBoxScore;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FPlayerBoxScore> AwayBoxScore;
    bool Validate(FString& OutError) const;
};
