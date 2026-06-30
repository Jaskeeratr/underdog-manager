#pragma once

#include "CoreMinimal.h"
#include "UnderdogCoreTypes.h"

class UNDERDOGSIMULATION_API FRivalryService
{
public:
    static void UpdateAfterGame(FLeagueState& League, const FGuid& HomeTeamId, const FGuid& AwayTeamId,
        int32 ScoreDiff, bool bPlayoffs);
    static void DecayRivalries(FLeagueState& League);
    static const FRivalry* GetRivalry(const FLeagueState& League, const FGuid& TeamA, const FGuid& TeamB);
    static int32 GetMoraleBonus(const FLeagueState& League, const FGuid& TeamId, const FGuid& OpponentId);
    static TArray<FRivalry> GetTopRivalries(const FLeagueState& League, int32 Count = 5);
};
