#include "LeagueGameSubsystem.h"
#include "FreeAgencyService.h"
#include "LeagueGenerator.h"
#include "LeagueService.h"
#include "ManagementService.h"
#include "OffseasonService.h"
#include "TradeService.h"
#include "UnderdogSaveGame.h"
#include "Kismet/GameplayStatics.h"

bool ULeagueGameSubsystem::StartNewLeague(int64 Seed, FString& OutError)
{
    FLeagueState Generated = FLeagueGenerator::Generate(static_cast<uint64>(Seed));
    if (!FLeagueGenerator::ValidateLeague(Generated, OutError)) { return false; }
    League = MoveTemp(Generated);
    if (League.Teams.Num() > 0) { FLeagueService::SetPlayerTeamId(League, League.Teams[0].TeamId); }
    OnLeagueChanged.Broadcast();
    return true;
}

bool ULeagueGameSubsystem::SimulateGame(const FGuid& GameId, FMatchResult& OutResult, FString& OutError)
{
    if (!FLeagueService::SimulateGame(League, GameId, OutResult, OutError)) { return false; }
    OnLeagueChanged.Broadcast();
    return true;
}

bool ULeagueGameSubsystem::AdvanceCurrentRound(TArray<FMatchResult>& OutResults, FString& OutError)
{
    if (!FLeagueService::AdvanceCurrentRound(League, OutResults, OutError)) { return false; }
    OnLeagueChanged.Broadcast();
    return true;
}

TArray<FTeamState> ULeagueGameSubsystem::GetStandings() const
{
    return FLeagueService::GetStandings(League);
}

bool ULeagueGameSubsystem::AutoBuildRotation(const FGuid& TeamId, FString& OutError)
{
    if (!FManagementService::AutoBuildRotation(League, TeamId, OutError)) { return false; }
    OnLeagueChanged.Broadcast();
    return true;
}

bool ULeagueGameSubsystem::SetTrainingPlan(const FGuid& TeamId,
    ETrainingFocus Focus, ETrainingIntensity Intensity, FString& OutError)
{
    if (!FManagementService::SetTrainingPlan(League, TeamId, Focus, Intensity, OutError)) { return false; }
    OnLeagueChanged.Broadcast();
    return true;
}

bool ULeagueGameSubsystem::AssignScout(const FGuid& TeamId, const FGuid& PlayerId, FString& OutError)
{
    if (!FManagementService::AssignScout(League, TeamId, PlayerId, OutError)) { return false; }
    OnLeagueChanged.Broadcast();
    return true;
}

bool ULeagueGameSubsystem::ProposeTrade(const FGuid& ProposingTeamId,
    const TArray<FGuid>& OutgoingPlayerIds, const FGuid& ReceivingTeamId,
    const TArray<FGuid>& IncomingPlayerIds, FString& OutError)
{
    if (!FTradeService::ProposeTrade(League, ProposingTeamId, OutgoingPlayerIds,
        ReceivingTeamId, IncomingPlayerIds, OutError)) { return false; }
    OnLeagueChanged.Broadcast();
    return true;
}

bool ULeagueGameSubsystem::AdvancePlayoffs(TArray<FMatchResult>& OutResults, FString& OutError)
{
    if (!FLeagueService::AdvancePlayoffs(League, OutResults, OutError)) { return false; }
    OnLeagueChanged.Broadcast();
    return true;
}

bool ULeagueGameSubsystem::StartOffseason(FString& OutError)
{
    if (!FOffseasonService::StartOffseason(League, OutError)) { return false; }
    OnLeagueChanged.Broadcast();
    return true;
}

bool ULeagueGameSubsystem::AdvanceOffseason(FString& OutError)
{
    if (!FOffseasonService::AdvanceOffseason(League, OutError)) { return false; }
    if (League.Phase == ESeasonPhase::RegularSeason)
    {
        FLeagueService::SetPlayerTeamId(League, League.Teams[0].TeamId);
    }
    OnLeagueChanged.Broadcast();
    return true;
}

bool ULeagueGameSubsystem::DraftPlayer(const FGuid& TeamId, int32 ProspectIndex, FString& OutError)
{
    if (!FOffseasonService::DraftPlayer(League, TeamId, ProspectIndex, OutError)) { return false; }
    OnLeagueChanged.Broadcast();
    return true;
}

bool ULeagueGameSubsystem::SignFreeAgent(const FGuid& TeamId, int32 FreeAgentIndex,
    int64 OfferSalary, int32 OfferYears, FString& OutError)
{
    if (!FFreeAgencyService::SignFreeAgent(League, TeamId, FreeAgentIndex, OfferSalary, OfferYears, OutError)) { return false; }
    OnLeagueChanged.Broadcast();
    return true;
}

bool ULeagueGameSubsystem::SetTactics(const FGuid& TeamId,
    EPaceStyle Pace, EOffenseStyle Offense, EDefenseStyle Defense, EReboundPriority Rebounding, FString& OutError)
{
    FTeamState* Team = League.Teams.FindByPredicate(
        [&TeamId](const FTeamState& T) { return T.TeamId == TeamId; });
    if (!Team) { OutError = TEXT("Team not found."); return false; }
    Team->Tactics.Pace = Pace;
    Team->Tactics.Offense = Offense;
    Team->Tactics.Defense = Defense;
    Team->Tactics.Rebounding = Rebounding;
    OnLeagueChanged.Broadcast();
    return true;
}

int32 ULeagueGameSubsystem::GetChemistry(const FGuid& TeamId) const
{
    const FTeamState* Team = League.Teams.FindByPredicate(
        [&TeamId](const FTeamState& T) { return T.TeamId == TeamId; });
    return Team ? Team->Chemistry : 50;
}

bool ULeagueGameSubsystem::SaveLeagueAsync(const FString& SlotName, FString& OutError)
{
    if (bSaveInProgress) { OutError = TEXT("A save operation is already running."); return false; }
    if (SlotName.IsEmpty() || League.Teams.Num() == 0) { OutError = TEXT("A slot name and active league are required."); return false; }
    UUnderdogSaveGame* Save = Cast<UUnderdogSaveGame>(UGameplayStatics::CreateSaveGameObject(UUnderdogSaveGame::StaticClass()));
    if (!Save) { OutError = TEXT("Could not allocate a save snapshot."); return false; }
    Save->SavedAtUtc = FDateTime::UtcNow();
    Save->League = League;
    bSaveInProgress = true;
    FAsyncSaveGameToSlotDelegate Delegate;
    Delegate.BindUObject(this, &ULeagueGameSubsystem::HandleSaveComplete);
    UGameplayStatics::AsyncSaveGameToSlot(Save, SlotName, 0, Delegate);
    return true;
}

void ULeagueGameSubsystem::HandleSaveComplete(const FString& SlotName, const int32 UserIndex, bool bSuccess)
{
    bSaveInProgress = false;
    OnSaveFinished.Broadcast(bSuccess, SlotName);
}

bool ULeagueGameSubsystem::LoadLeague(const FString& SlotName, FString& OutError)
{
    UUnderdogSaveGame* Save = Cast<UUnderdogSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
    if (!Save) { OutError = TEXT("Save slot could not be loaded."); return false; }
    if (Save->SchemaVersion != UUnderdogSaveGame::CurrentSchemaVersion)
    {
        OutError = TEXT("Save schema is not supported by this build.");
        return false;
    }
    if (!FLeagueGenerator::ValidateLeague(Save->League, OutError)) { return false; }
    League = Save->League;
    if (League.Teams.Num() > 0) { FLeagueService::SetPlayerTeamId(League, League.Teams[0].TeamId); }
    OnLeagueChanged.Broadcast();
    return true;
}
