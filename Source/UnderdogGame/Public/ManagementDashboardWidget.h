#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ManagementDashboardWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;
struct FLeagueState;
struct FTeamState;
struct FMatchResult;
struct FMatchSnapshot;
struct FPlayoffBracket;
struct FSeasonAward;
enum class ETrainingFocus : uint8;
enum class ETrainingIntensity : uint8;
enum class EPaceStyle : uint8;
enum class EOffenseStyle : uint8;
enum class EDefenseStyle : uint8;
enum class EReboundPriority : uint8;

UCLASS()
class UNDERDOGGAME_API UManagementDashboardWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;

private:
    UPROPERTY() TObjectPtr<UTextBlock> ClubNameText;
    UPROPERTY() TObjectPtr<UTextBlock> SeasonText;
    UPROPERTY() TObjectPtr<UTextBlock> RecordText;
    UPROPERTY() TObjectPtr<UTextBlock> ChemistryText;
    UPROPERTY() TObjectPtr<UTextBlock> SalaryCapText;
    UPROPERTY() TObjectPtr<UTextBlock> NextOpponentText;
    UPROPERTY() TObjectPtr<UTextBlock> MatchupDetailsText;
    UPROPERTY() TObjectPtr<UTextBlock> StatusText;
    UPROPERTY() TObjectPtr<UVerticalBox> RosterList;
    UPROPERTY() TObjectPtr<UVerticalBox> StandingsList;
    UPROPERTY() TObjectPtr<UButton> SimulateButton;
    UPROPERTY() TObjectPtr<UTextBlock> SimulateButtonText;
    UPROPERTY() TObjectPtr<UWidgetSwitcher> ScreenSwitcher;
    UPROPERTY() TObjectPtr<UVerticalBox> RosterDetailList;
    UPROPERTY() TObjectPtr<UVerticalBox> ScheduleList;
    UPROPERTY() TObjectPtr<UVerticalBox> StandingsDetailList;
    UPROPERTY() TObjectPtr<UVerticalBox> ScoutingList;
    UPROPERTY() TObjectPtr<UVerticalBox> TrainingList;
    UPROPERTY() TObjectPtr<UVerticalBox> TradeList;
    UPROPERTY() TObjectPtr<UVerticalBox> PlayoffList;
    UPROPERTY() TObjectPtr<UVerticalBox> ResultsList;
    UPROPERTY() TObjectPtr<UVerticalBox> AwardsList;
    UPROPERTY() TObjectPtr<UVerticalBox> OffseasonList;
    UPROPERTY() TObjectPtr<UVerticalBox> TacticsList;
    UPROPERTY() TObjectPtr<UVerticalBox> SaveLoadList;
    UPROPERTY() TObjectPtr<UVerticalBox> GameRecapList;
    UPROPERTY() TObjectPtr<UVerticalBox> HistoryList;
    UPROPERTY() TObjectPtr<UVerticalBox> ContractsList;
    UPROPERTY() TObjectPtr<UVerticalBox> RivalriesList;
    UPROPERTY() TObjectPtr<UTextBlock> TrainingPlanText;
    UPROPERTY() TObjectPtr<UTextBlock> ScoutingStatusText;
    FGuid RecommendedScoutPlayerId;

    FGuid SelectedTradeTeamId;
    TArray<FGuid> TradeOutgoingPlayerIds;
    TArray<FGuid> TradeIncomingPlayerIds;
    TArray<FGuid> DisplayedOutgoingIds;
    TArray<FGuid> DisplayedIncomingIds;
    TArray<FGuid> DisplayedTeamIds;

    TArray<int32> DisplayedDraftIndices;
    TArray<FGuid> DisplayedExtensionPlayerIds;

    void BuildLayout();
    void RefreshDashboard();
    UTextBlock* MakeText(const FString& Text, int32 Size, const FLinearColor& Color, bool bBold = false);
    UBorder* MakeCard(const FLinearColor& Color);
    UButton* MakeNavigationButton(const FString& Label, bool bActive);
    UVerticalBox* MakeDetailScreen(const FString& Eyebrow, const FString& Title, const FString& Description,
        TObjectPtr<UVerticalBox>& OutList);
    void RefreshRosterScreen(const FLeagueState& League, const FTeamState& Club);
    void RefreshScheduleScreen(const FLeagueState& League, const FTeamState& Club);
    void RefreshStandingsScreen(const TArray<FTeamState>& Standings, const FGuid& ClubId);
    void RefreshScoutingScreen(const FLeagueState& League, const FTeamState& Club);
    void RefreshTrainingScreen(const FTeamState& Club);
    void RefreshTacticsScreen(const FTeamState& Club);
    void RefreshTradeScreen(const FLeagueState& League, const FTeamState& Club);
    void RefreshPlayoffScreen(const FLeagueState& League, const FTeamState& Club);
    void RefreshResultsScreen(const TArray<FMatchResult>& Results, const FLeagueState& League, const FTeamState& Club);
    void RefreshAwardsScreen(const FLeagueState& League, const FTeamState& Club);
    void RefreshOffseasonScreen(const FLeagueState& League, const FTeamState& Club);
    void RefreshSaveLoadScreen();
    void RefreshGameRecapScreen(const FLeagueState& League, const FTeamState& Club);
    void RefreshHistoryScreen(const FLeagueState& League);
    void RefreshContractsScreen(const FLeagueState& League, const FTeamState& Club);
    void RefreshRivalriesScreen(const FLeagueState& League, const FTeamState& Club);
    void OfferExtensionAtSlot(int32 Slot);
    void SetScreen(int32 Index);
    void ApplyTrainingPlan(ETrainingFocus Focus, ETrainingIntensity Intensity);
    void ApplyTactics(EPaceStyle Pace, EOffenseStyle Offense, EDefenseStyle Defense, EReboundPriority Rebounding);
    void DraftProspectAtSlot(int32 Slot);
    void SelectTradeTeamAtSlot(int32 Slot);
    void ToggleTradeOutgoingAtSlot(int32 Slot);
    void ToggleTradeIncomingAtSlot(int32 Slot);
    void SaveToSlot(int32 Slot);
    void LoadFromSlot(int32 Slot);

    UFUNCTION() void HandleSimulateRound();
    UFUNCTION() void ShowOverview();
    UFUNCTION() void ShowRoster();
    UFUNCTION() void ShowSchedule();
    UFUNCTION() void ShowStandings();
    UFUNCTION() void ShowScouting();
    UFUNCTION() void ShowTraining();
    UFUNCTION() void ShowTrades();
    UFUNCTION() void ShowPlayoffs();
    UFUNCTION() void ShowTactics();
    UFUNCTION() void ShowAwards();
    UFUNCTION() void ShowOffseason();
    UFUNCTION() void ShowSaveLoad();
    UFUNCTION() void ShowGameRecap();
    UFUNCTION() void ShowHistory();
    UFUNCTION() void ShowContracts();
    UFUNCTION() void ShowRivalries();
    UFUNCTION() void HandleAdvanceOffseason();
    UFUNCTION() void HandleSignFreeAgent();
    UFUNCTION() void HandleBuildTrade();
    UFUNCTION() void HandleClearTrade();
    UFUNCTION() void SetTacticsPaceSlow();
    UFUNCTION() void SetTacticsPaceBalanced();
    UFUNCTION() void SetTacticsPaceFast();
    UFUNCTION() void SetTacticsOffenseInside();
    UFUNCTION() void SetTacticsOffenseBalanced();
    UFUNCTION() void SetTacticsOffensePerimeter();
    UFUNCTION() void SetTacticsDefenseMan();
    UFUNCTION() void SetTacticsDefenseSwitching();
    UFUNCTION() void SetTacticsDefenseZone();
    UFUNCTION() void SetTacticsReboundTransition();
    UFUNCTION() void SetTacticsReboundBalanced();
    UFUNCTION() void SetTacticsReboundCrash();
    UFUNCTION() void HandleAutoRotation();
    UFUNCTION() void HandleAssignScout();
    UFUNCTION() void HandleProposeTrade();
    UFUNCTION() void SetTrainingBalanced();
    UFUNCTION() void SetTrainingShooting();
    UFUNCTION() void SetTrainingDefense();
    UFUNCTION() void SetTrainingConditioning();
    UFUNCTION() void SetTrainingRecovery();
    UFUNCTION() void SetTrainingHigh();
    UFUNCTION() void HandleDraft0();
    UFUNCTION() void HandleDraft1();
    UFUNCTION() void HandleDraft2();
    UFUNCTION() void HandleDraft3();
    UFUNCTION() void HandleDraft4();
    UFUNCTION() void HandleDraft5();
    UFUNCTION() void HandleDraft6();
    UFUNCTION() void HandleDraft7();
    UFUNCTION() void HandleTeam0();
    UFUNCTION() void HandleTeam1();
    UFUNCTION() void HandleTeam2();
    UFUNCTION() void HandleTeam3();
    UFUNCTION() void HandleTeam4();
    UFUNCTION() void HandleTeam5();
    UFUNCTION() void HandleTeam6();
    UFUNCTION() void HandleTeam7();
    UFUNCTION() void HandleOut0();
    UFUNCTION() void HandleOut1();
    UFUNCTION() void HandleOut2();
    UFUNCTION() void HandleOut3();
    UFUNCTION() void HandleOut4();
    UFUNCTION() void HandleOut5();
    UFUNCTION() void HandleOut6();
    UFUNCTION() void HandleOut7();
    UFUNCTION() void HandleIn0();
    UFUNCTION() void HandleIn1();
    UFUNCTION() void HandleIn2();
    UFUNCTION() void HandleIn3();
    UFUNCTION() void HandleIn4();
    UFUNCTION() void HandleIn5();
    UFUNCTION() void HandleIn6();
    UFUNCTION() void HandleIn7();
    UFUNCTION() void HandleSaveSlot0();
    UFUNCTION() void HandleSaveSlot1();
    UFUNCTION() void HandleSaveSlot2();
    UFUNCTION() void HandleLoadSlot0();
    UFUNCTION() void HandleLoadSlot1();
    UFUNCTION() void HandleLoadSlot2();
    UFUNCTION() void HandleExtend0();
    UFUNCTION() void HandleExtend1();
    UFUNCTION() void HandleExtend2();
    UFUNCTION() void HandleExtend3();
    UFUNCTION() void HandleExtend4();
    UFUNCTION() void HandleExtend5();
    UFUNCTION() void HandleExtend6();
    UFUNCTION() void HandleExtend7();

    TArray<FMatchResult> LastRoundResults;
    TArray<FMatchSnapshot> LastRoundSnapshots;
};
