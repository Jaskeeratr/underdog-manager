#pragma once

#include "CoreMinimal.h"
#include "UnderdogCoreTypes.h"

class UNDERDOGSIMULATION_API FLeagueService
{
public:
    static bool SimulateGame(FLeagueState& League, const FGuid& GameId, FMatchResult& OutResult, FString& OutError);
    static bool AdvanceCurrentRound(FLeagueState& League, TArray<FMatchResult>& OutResults, FString& OutError);
    static TArray<FTeamState> GetStandings(const FLeagueState& League);

private:
    static bool BuildSnapshot(const FLeagueState& League, const FScheduledGame& Game, FMatchSnapshot& OutSnapshot, FString& OutError);
    static void ApplyResult(FLeagueState& League, FScheduledGame& Game, const FMatchResult& Result);
    static void ApplyPlayerConsequences(FTeamState& Team, const TArray<FPlayerBoxScore>& BoxScore, bool bWon, uint64 Seed);
};
