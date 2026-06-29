#pragma once

#include "CoreMinimal.h"
#include "UnderdogCoreTypes.h"

class UNDERDOGSIMULATION_API FAwardsService
{
public:
    static void AccumulateBoxScore(FLeagueState& League, const FMatchResult& Result,
        const FGuid& HomeTeamId, const FGuid& AwayTeamId);
    static TArray<FSeasonAward> CalculateAwards(const FLeagueState& League);
    static FSeasonAward CalculateChampionMVP(const FLeagueState& League);
};
