#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UnderdogCoreTypes.h"
#include "LeagueGameSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeagueChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSaveFinished, bool, bSuccess, const FString&, SlotName);

UCLASS()
class UNDERDOGGAME_API ULeagueGameSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable) FOnLeagueChanged OnLeagueChanged;
    UPROPERTY(BlueprintAssignable) FOnSaveFinished OnSaveFinished;

    UFUNCTION(BlueprintCallable, Category="Underdog|League") bool StartNewLeague(int64 Seed, FString& OutError);
    UFUNCTION(BlueprintPure, Category="Underdog|League") FLeagueState GetLeague() const { return League; }
    UFUNCTION(BlueprintCallable, Category="Underdog|League") bool SimulateGame(const FGuid& GameId, FMatchResult& OutResult, FString& OutError);
    UFUNCTION(BlueprintCallable, Category="Underdog|League") bool AdvanceCurrentRound(TArray<FMatchResult>& OutResults, FString& OutError);
    UFUNCTION(BlueprintPure, Category="Underdog|League") TArray<FTeamState> GetStandings() const;
    UFUNCTION(BlueprintCallable, Category="Underdog|Roster") bool AutoBuildRotation(const FGuid& TeamId, FString& OutError);
    UFUNCTION(BlueprintCallable, Category="Underdog|Training") bool SetTrainingPlan(const FGuid& TeamId,
        ETrainingFocus Focus, ETrainingIntensity Intensity, FString& OutError);
    UFUNCTION(BlueprintCallable, Category="Underdog|Scouting") bool AssignScout(const FGuid& TeamId,
        const FGuid& PlayerId, FString& OutError);
    UFUNCTION(BlueprintCallable, Category="Underdog|Trade") bool ProposeTrade(const FGuid& ProposingTeamId,
        const TArray<FGuid>& OutgoingPlayerIds, const FGuid& ReceivingTeamId,
        const TArray<FGuid>& IncomingPlayerIds, FString& OutError);
    UFUNCTION(BlueprintPure, Category="Underdog|Trade") TArray<FTradeOffer> GetTradeHistory() const { return League.TradeHistory; }
    UFUNCTION(BlueprintCallable, Category="Underdog|Playoffs") bool AdvancePlayoffs(TArray<FMatchResult>& OutResults, FString& OutError);
    UFUNCTION(BlueprintPure, Category="Underdog|Playoffs") FPlayoffBracket GetPlayoffBracket() const { return League.Playoffs; }
    UFUNCTION(BlueprintPure, Category="Underdog|League") ESeasonPhase GetSeasonPhase() const { return League.Phase; }
    UFUNCTION(BlueprintPure, Category="Underdog|League") int32 GetSeasonNumber() const { return League.SeasonNumber; }
    UFUNCTION(BlueprintCallable, Category="Underdog|Offseason") bool StartOffseason(FString& OutError);
    UFUNCTION(BlueprintCallable, Category="Underdog|Offseason") bool AdvanceOffseason(FString& OutError);
    UFUNCTION(BlueprintCallable, Category="Underdog|Offseason") bool DraftPlayer(const FGuid& TeamId, int32 ProspectIndex, FString& OutError);
    UFUNCTION(BlueprintPure, Category="Underdog|Offseason") FOffseasonState GetOffseasonState() const { return League.Offseason; }
    UFUNCTION(BlueprintPure, Category="Underdog|Awards") TArray<FSeasonAward> GetAwards() const { return League.Awards; }
    UFUNCTION(BlueprintPure, Category="Underdog|Stats") TArray<FSeasonStats> GetSeasonStats() const { return League.SeasonStats; }
    UFUNCTION(BlueprintCallable, Category="Underdog|Save") bool SaveLeagueAsync(const FString& SlotName, FString& OutError);
    UFUNCTION(BlueprintCallable, Category="Underdog|Save") bool LoadLeague(const FString& SlotName, FString& OutError);
    UFUNCTION(BlueprintPure, Category="Underdog|League") bool HasLeague() const { return League.Teams.Num() > 0; }

private:
    UPROPERTY() FLeagueState League;
    bool bSaveInProgress = false;
    void HandleSaveComplete(const FString& SlotName, const int32 UserIndex, bool bSuccess);
};
