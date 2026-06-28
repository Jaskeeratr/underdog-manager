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
    UFUNCTION(BlueprintCallable, Category="Underdog|Save") bool SaveLeagueAsync(const FString& SlotName, FString& OutError);
    UFUNCTION(BlueprintCallable, Category="Underdog|Save") bool LoadLeague(const FString& SlotName, FString& OutError);
    UFUNCTION(BlueprintPure, Category="Underdog|League") bool HasLeague() const { return League.Teams.Num() > 0; }

private:
    UPROPERTY() FLeagueState League;
    bool bSaveInProgress = false;
    void HandleSaveComplete(const FString& SlotName, const int32 UserIndex, bool bSuccess);
};
