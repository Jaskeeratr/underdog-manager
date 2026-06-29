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

UENUM(BlueprintType)
enum class ESeasonPhase : uint8 { RegularSeason, Playoffs, Complete };

UENUM(BlueprintType)
enum class ETradeStatus : uint8 { Pending, Accepted, Rejected, Expired };

UENUM(BlueprintType)
enum class EAwardType : uint8 { MVP, DPOY, ROY, MIP, ChampionMVP };

UENUM(BlueprintType)
enum class EOffseasonStep : uint8 { Awards, Aging, ContractExpiry, Draft, Resigning, Complete };

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
struct UNDERDOGCORE_API FTradeAsset
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid PlayerId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid FromTeamId;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FTradeOffer
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid TradeId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid ProposingTeamId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid ReceivingTeamId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FTradeAsset> Outgoing;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FTradeAsset> Incoming;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) ETradeStatus Status = ETradeStatus::Pending;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ProposedRound = 0;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FPlayoffSeries
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid SeriesId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 BracketSlot = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 PlayoffRound = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid HigherSeedTeamId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid LowerSeedTeamId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 HigherSeedWins = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 LowerSeedWins = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FGuid> GameIds;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bComplete = false;
    FGuid GetWinnerId() const { return HigherSeedWins >= 4 ? HigherSeedTeamId : LowerSeedWins >= 4 ? LowerSeedTeamId : FGuid(); }
    int32 GamesPlayed() const { return HigherSeedWins + LowerSeedWins; }
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FPlayoffBracket
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FPlayoffSeries> Series;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid ChampionTeamId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 CurrentPlayoffRound = 0;
    bool IsComplete() const { return ChampionTeamId.IsValid(); }
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FSeasonStats
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid PlayerId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 GamesPlayed = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 TotalPoints = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 TotalRebounds = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 TotalAssists = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 TotalSteals = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 TotalBlocks = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 TotalTurnovers = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 OverallAtSeasonStart = 0;
    float PPG() const { return GamesPlayed > 0 ? static_cast<float>(TotalPoints) / GamesPlayed : 0.0f; }
    float RPG() const { return GamesPlayed > 0 ? static_cast<float>(TotalRebounds) / GamesPlayed : 0.0f; }
    float APG() const { return GamesPlayed > 0 ? static_cast<float>(TotalAssists) / GamesPlayed : 0.0f; }
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FSeasonAward
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EAwardType Type = EAwardType::MVP;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid PlayerId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid TeamId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Season = 1;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FCommentaryLine
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Text;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Period = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ClockSeconds = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Importance = 0;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FDraftProspect
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FPlayerProfile Profile;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 DraftOrder = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bDrafted = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid DraftedByTeamId;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FOffseasonState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EOffseasonStep CurrentStep = EOffseasonStep::Awards;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FDraftProspect> DraftClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FGuid> ExpiredContractPlayerIds;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 CurrentDraftPick = 0;
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
    UPROPERTY(EditAnywhere, BlueprintReadOnly) ESeasonPhase Phase = ESeasonPhase::RegularSeason;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 TradeDeadlineRound = 16;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FTradeOffer> TradeHistory;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FPlayoffBracket Playoffs;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 SeasonNumber = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FSeasonStats> SeasonStats;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FSeasonAward> Awards;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FOffseasonState Offseason;
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
