#include "LeagueGameSubsystem.h"
#include "LeagueGenerator.h"
#include "MatchSimulator.h"
#include "UnderdogSaveGame.h"
#include "Kismet/GameplayStatics.h"

bool ULeagueGameSubsystem::StartNewLeague(int64 Seed, FString& OutError)
{
    FLeagueState Generated = FLeagueGenerator::Generate(static_cast<uint64>(Seed));
    if (!FLeagueGenerator::ValidateLeague(Generated, OutError)) { return false; }
    League = MoveTemp(Generated);
    OnLeagueChanged.Broadcast();
    return true;
}

bool ULeagueGameSubsystem::SimulateGame(const FGuid& GameId, FMatchResult& OutResult, FString& OutError)
{
    FScheduledGame* Game = League.Schedule.FindByPredicate([&GameId](const FScheduledGame& Candidate) { return Candidate.GameId == GameId; });
    if (!Game || Game->bComplete) { OutError = TEXT("The requested game does not exist or is already complete."); return false; }
    FTeamState* Home = League.Teams.FindByPredicate([Game](const FTeamState& Team) { return Team.TeamId == Game->HomeTeamId; });
    FTeamState* Away = League.Teams.FindByPredicate([Game](const FTeamState& Team) { return Team.TeamId == Game->AwayTeamId; });
    if (!Home || !Away) { OutError = TEXT("The scheduled teams could not be resolved."); return false; }

    FMatchSnapshot Snapshot;
    Snapshot.GameId = Game->GameId;
    Snapshot.SimulationVersion = League.SimulationVersion;
    Snapshot.Seed = static_cast<int64>(GetTypeHash(Game->GameId) ^ static_cast<uint64>(League.LeagueSeed));
    Snapshot.HomeTeam = *Home;
    Snapshot.AwayTeam = *Away;
    FPossessionMatchSimulator Simulator;
    OutResult = Simulator.Simulate(Snapshot);
    if (!OutResult.Validate(OutError)) { return false; }

    Game->bComplete = true;
    Home->PointsFor += OutResult.HomeScore; Home->PointsAgainst += OutResult.AwayScore;
    Away->PointsFor += OutResult.AwayScore; Away->PointsAgainst += OutResult.HomeScore;
    if (OutResult.HomeScore > OutResult.AwayScore) { Home->Wins++; Away->Losses++; }
    else { Away->Wins++; Home->Losses++; }
    OnLeagueChanged.Broadcast();
    return true;
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
    OnLeagueChanged.Broadcast();
    return true;
}
