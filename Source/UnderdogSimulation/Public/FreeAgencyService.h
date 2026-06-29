#pragma once

#include "CoreMinimal.h"
#include "UnderdogCoreTypes.h"

class UNDERDOGSIMULATION_API FFreeAgencyService
{
public:
    static void BuildFreeAgentPool(FLeagueState& League);
    static bool SignFreeAgent(FLeagueState& League, const FGuid& TeamId,
        int32 FreeAgentIndex, int64 OfferSalary, int32 OfferYears, FString& OutError);
    static void AutoSignFreeAgents(FLeagueState& League);
    static int64 CalculateAskingPrice(const FPlayerProfile& Player);
    static bool CanAfford(const FTeamState& Team, int64 Salary);
};
