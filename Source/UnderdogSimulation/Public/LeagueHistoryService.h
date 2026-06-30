#pragma once

#include "CoreMinimal.h"
#include "UnderdogCoreTypes.h"

class UNDERDOGSIMULATION_API FLeagueHistoryService
{
public:
    static void RecordSeasonEnd(FLeagueState& League);
    static void AccumulatePlayerStats(FLeagueState& League);
    static TArray<FAllTimeLeader> GetTopScorers(const FLeagueHistory& History, int32 Count = 10);
    static TArray<FAllTimeLeader> GetTopRebounders(const FLeagueHistory& History, int32 Count = 10);
    static TArray<FAllTimeLeader> GetTopAssisters(const FLeagueHistory& History, int32 Count = 10);
};
