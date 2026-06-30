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
enum class EOffseasonStep : uint8 { Awards, Aging, ContractExpiry, FreeAgency, Draft, Resigning, Complete };

UENUM(BlueprintType)
enum class EFacilityType : uint8 { TrainingCentre, MedicalCentre, ScoutingDepartment, ArenaOperations };

UENUM(BlueprintType)
enum class EOwnerObjectiveType : uint8 { WinGames, ReachPlayoffs, PositiveBalance, ImproveChemistry };

UENUM(BlueprintType)
enum class EStaffRole : uint8
{
    HeadCoach, OffensiveCoach, DefensiveCoach, DevelopmentCoach, HeadScout, MedicalDirector
};

UENUM(BlueprintType)
enum class EStaffPersonality : uint8 { PlayersCoach, Strategist, Developer, Traditionalist, Negotiator };

UENUM(BlueprintType)
enum class EManagerEmploymentStatus : uint8 { Employed, Unemployed };

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
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString InjuryDescription;
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
struct UNDERDOGCORE_API FFranchiseFinances
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 CashMinorUnits = 2500000000LL;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 TicketPriceMinorUnits = 4500;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 SeasonRevenueMinorUnits = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 SeasonExpensesMinorUnits = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 LastGameRevenueMinorUnits = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 LastAttendance = 0;
    int64 OperatingProfit() const { return SeasonRevenueMinorUnits - SeasonExpensesMinorUnits; }
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FFanbaseState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Support = 45;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Reputation = 35;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 SeasonTicketHolders = 3500;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FFacilityState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EFacilityType Type = EFacilityType::TrainingCentre;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Level = 1;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FOwnerObjective
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EOwnerObjectiveType Type = EOwnerObjectiveType::WinGames;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Target = 8;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Current = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bCompleted = false;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FOwnershipState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Confidence = 60;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FOwnerObjective> Objectives;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FFranchiseState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FFranchiseFinances Finances;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FFanbaseState Fanbase;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FFacilityState> Facilities;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FOwnershipState Ownership;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FStaffRatings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Offense = 50;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Defense = 50;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Development = 50;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Motivation = 50;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Scouting = 50;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Medical = 50;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 TacticalFlexibility = 50;
    int32 OverallForRole(EStaffRole Role) const;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FStaffContract
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 SalaryMinorUnits = 75000000LL;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 YearsRemaining = 2;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FStaffMember
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid StaffId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Age = 42;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EStaffRole Role = EStaffRole::HeadCoach;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EStaffPersonality Personality = EStaffPersonality::Strategist;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FStaffRatings Ratings;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FStaffContract Contract;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Reputation = 50;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FOrganizationState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FStaffMember> Staff;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 StaffChemistry = 55;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 TacticalFamiliarity = 50;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 AnnualPayrollMinorUnits = 0;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FTradeEvaluation
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bLegal = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bAccepted = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ProposerReceivesValue = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ReceiverReceivesValue = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 ProposerSalaryChange = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 ReceiverSalaryChange = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Summary;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FString> Reasons;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FManagerSeasonRecord
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Season = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid TeamId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString TeamName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Wins = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Losses = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bReachedPlayoffs = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bChampion = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 OwnerConfidence = 50;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FManagerJobOffer
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid TeamId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString TeamName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ContractYears = 2;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ExpectedWins = 8;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FManagerCareer
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString ManagerName = TEXT("J. Rai");
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid CurrentTeamId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EManagerEmploymentStatus EmploymentStatus = EManagerEmploymentStatus::Employed;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ContractYearsRemaining = 2;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 CareerWins = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 CareerLosses = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 PlayoffAppearances = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Championships = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 CareerScore = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FManagerSeasonRecord> SeasonHistory;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FManagerJobOffer> JobOffers;
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
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Chemistry = 50;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 WinStreak = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Wins = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Losses = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 PointsFor = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 PointsAgainst = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FFranchiseState Franchise;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FOrganizationState Organization;
    int64 TotalSalary() const { int64 Sum = 0; for (const FPlayerProfile& P : Players) { Sum += P.Contract.SalaryMinorUnits; } return Sum; }
    bool IsOverCap() const { return TotalSalary() > SalaryCapMinorUnits; }
    int64 LuxuryTaxThreshold() const { return SalaryCapMinorUnits + SalaryCapMinorUnits / 4; }
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
    FGuid GetWinnerId() const { return HigherSeedWins >= 2 ? HigherSeedTeamId : LowerSeedWins >= 2 ? LowerSeedTeamId : FGuid(); }
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
struct UNDERDOGCORE_API FFreeAgent
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FPlayerProfile Profile;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid PreviousTeamId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 AskingSalary = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bSigned = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid SignedByTeamId;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FMentorship
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid VeteranId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid RookieId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 BonusRatingGain = 0;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FOffseasonState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EOffseasonStep CurrentStep = EOffseasonStep::Awards;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FDraftProspect> DraftClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FGuid> ExpiredContractPlayerIds;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FFreeAgent> FreeAgentPool;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 CurrentDraftPick = 0;
};

UENUM(BlueprintType)
enum class EHighlightTemplate : uint8
{
    ThreePointer, DriveAndFinish, AssistedBasket, BlockPlay,
    StealFastBreak, FreeThrows, ClutchBasket, FinalPossession, GenericFallback
};

UENUM(BlueprintType)
enum class ECameraPreset : uint8
{
    Wide, Baseline, FollowBall, CloseUp, HighAngle, ReverseAngle, Broadcast
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FHighlightCue
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EHighlightTemplate Template = EHighlightTemplate::GenericFallback;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) ECameraPreset Camera = ECameraPreset::Broadcast;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Period = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ClockSeconds = 720;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 HomeScoreBefore = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 AwayScoreBefore = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 HomeScoreAfter = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 AwayScoreAfter = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid PossessionTeamId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid PrimaryPlayerId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid SecondaryPlayerId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString PrimaryPlayerName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString SecondaryPlayerName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Description;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Importance = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float PlaybackDuration = 4.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bOutcome = true;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FTeamPresentationData
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid TeamId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString City;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Nickname;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString FullName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Wins = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Losses = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 WinStreak = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Chemistry = 50;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FPlayerProfile> StartingFive;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FTeamTactics Tactics;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FString> InjuredPlayers;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 RivalryIntensity = 0;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FQuarterScore
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 HomePoints = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 AwayPoints = 0;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FPlayByPlayEntry
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Period = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ClockSeconds = 720;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Description;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 HomeScore = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 AwayScore = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bHighlight = false;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FGameRecap
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid GameId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FQuarterScore> QuarterScores;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FPlayByPlayEntry> PlayByPlay;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid HomeTeamId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid AwayTeamId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString HomeTeamName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString AwayTeamName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 FinalHomeScore = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 FinalAwayScore = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Headline;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Summary;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FChampionshipRecord
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Season = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid ChampionTeamId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString ChampionName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid MvpPlayerId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString MvpName;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FAllTimeLeader
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid PlayerId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString PlayerName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 TotalPoints = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 TotalRebounds = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 TotalAssists = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 GamesPlayed = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 SeasonsPlayed = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Championships = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 MvpAwards = 0;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FLeagueHistory
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FChampionshipRecord> Championships;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FAllTimeLeader> AllTimeLeaders;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FSeasonAward> AllAwards;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FExtensionOffer
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid PlayerId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString PlayerName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 OfferedSalary = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 OfferedYears = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 AskingSalary = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 AskingYears = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bAccepted = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bDeclined = false;
};

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FRivalry
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid TeamAId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid TeamBId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Intensity = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 PlayoffMeetings = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 CloseGames = 0;
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
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FMentorship> Mentorships;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FOffseasonState Offseason;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FLeagueHistory History;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FRivalry> Rivalries;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FStaffMember> StaffMarket;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FManagerCareer ManagerCareer;
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

USTRUCT(BlueprintType)
struct UNDERDOGCORE_API FMatchPresentationPackage
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGuid GameId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FTeamPresentationData Home;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FTeamPresentationData Away;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FHighlightCue> Highlights;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FMatchResult Result;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGameRecap Recap;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 RivalryIntensity = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bPlayoffGame = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString StandingsImplication;
};
