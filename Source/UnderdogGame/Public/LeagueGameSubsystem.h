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
    UFUNCTION(BlueprintCallable, Category="Underdog|FreeAgency") bool SignFreeAgent(const FGuid& TeamId, int32 FreeAgentIndex,
        int64 OfferSalary, int32 OfferYears, FString& OutError);
    UFUNCTION(BlueprintCallable, Category="Underdog|Tactics") bool SetTactics(const FGuid& TeamId,
        EPaceStyle Pace, EOffenseStyle Offense, EDefenseStyle Defense, EReboundPriority Rebounding, FString& OutError);
    UFUNCTION(BlueprintPure, Category="Underdog|FreeAgency") TArray<FFreeAgent> GetFreeAgentPool() const { return League.Offseason.FreeAgentPool; }
    UFUNCTION(BlueprintPure, Category="Underdog|Chemistry") int32 GetChemistry(const FGuid& TeamId) const;
    UFUNCTION(BlueprintPure, Category="Underdog|Chemistry") TArray<FMentorship> GetMentorships() const { return League.Mentorships; }
    UFUNCTION(BlueprintPure, Category="Underdog|History") FLeagueHistory GetLeagueHistory() const { return League.History; }
    UFUNCTION(BlueprintPure, Category="Underdog|Rivalry") TArray<FRivalry> GetRivalries() const { return League.Rivalries; }
    UFUNCTION(BlueprintCallable, Category="Underdog|Contract") bool OfferExtension(const FGuid& TeamId,
        const FGuid& PlayerId, int64 OfferedSalary, int32 OfferedYears, FString& OutError);
    UFUNCTION(BlueprintPure, Category="Underdog|Contract") TArray<FExtensionOffer> GetEligibleExtensions(const FGuid& TeamId) const;
    FGameRecap BuildGameRecap(const FMatchResult& Result, const FMatchSnapshot& Snapshot) const;
    FMatchPresentationPackage BuildPresentationPackage(const FMatchResult& Result, const FMatchSnapshot& Snapshot) const;
    FMatchPresentationPackage SimulateAndBuildPresentation(const FGuid& GameId, FString& OutError);
    UFUNCTION(BlueprintCallable, Category="Underdog|Save") bool SaveLeagueAsync(const FString& SlotName, FString& OutError);
    UFUNCTION(BlueprintCallable, Category="Underdog|Save") bool LoadLeague(const FString& SlotName, FString& OutError);
    UFUNCTION(BlueprintCallable, Category="Underdog|Save") bool DeleteSaveSlot(const FString& SlotName);
    UFUNCTION(BlueprintPure, Category="Underdog|Save") bool GetSaveSlotInfo(const FString& SlotName,
        int32& OutSeason, int32& OutRound, FString& OutTeamName, FDateTime& OutSavedAt) const;
    UFUNCTION(BlueprintPure, Category="Underdog|League") bool HasLeague() const { return League.Teams.Num() > 0; }
    UFUNCTION(BlueprintPure, Category="Underdog|Franchise") FFranchiseState GetFranchiseState(const FGuid& TeamId) const;
    UFUNCTION(BlueprintCallable, Category="Underdog|Franchise") bool SetTicketPrice(const FGuid& TeamId,
        int64 TicketPriceMinorUnits, FString& OutError);
    UFUNCTION(BlueprintCallable, Category="Underdog|Franchise") bool UpgradeFacility(const FGuid& TeamId,
        EFacilityType Type, FString& OutError);
    UFUNCTION(BlueprintPure, Category="Underdog|Franchise") int64 GetFacilityUpgradeCost(const FGuid& TeamId,
        EFacilityType Type) const;
    UFUNCTION(BlueprintPure, Category="Underdog|Staff") TArray<FStaffMember> GetStaffMarket() const { return League.StaffMarket; }
    UFUNCTION(BlueprintCallable, Category="Underdog|Staff") bool HireStaff(const FGuid& TeamId,
        const FGuid& StaffId, int64 OfferedSalary, int32 OfferedYears, FString& OutError);
    UFUNCTION(BlueprintCallable, Category="Underdog|Staff") bool FireStaff(const FGuid& TeamId,
        EStaffRole Role, FString& OutError);
    UFUNCTION(BlueprintPure, Category="Underdog|Career") FManagerCareer GetManagerCareer() const { return League.ManagerCareer; }
    UFUNCTION(BlueprintCallable, Category="Underdog|Career") bool AcceptManagerJob(const FGuid& TeamId, FString& OutError);
    UFUNCTION(BlueprintCallable, Category="Underdog|Career") bool SetManagerName(const FString& ManagerName, FString& OutError);

private:
    UPROPERTY() FLeagueState League;
    bool bSaveInProgress = false;
    void HandleSaveComplete(const FString& SlotName, const int32 UserIndex, bool bSuccess);
};
