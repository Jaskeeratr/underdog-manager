#pragma once

#include "CoreMinimal.h"
#include "UnderdogCoreTypes.h"

class UNDERDOGSIMULATION_API FLeagueGenerator
{
public:
    static FLeagueState Generate(uint64 Seed);
    static TArray<FScheduledGame> GenerateDoubleRoundRobin(const TArray<FTeamState>& Teams, uint64 Seed);
    static bool ValidateLeague(const FLeagueState& League, FString& OutError);
};
