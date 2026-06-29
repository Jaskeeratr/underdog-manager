#pragma once

#include "CoreMinimal.h"
#include "UnderdogCoreTypes.h"

class UNDERDOGSIMULATION_API FChemistryService
{
public:
    static int32 CalculateChemistry(const FTeamState& Team, const FLeagueState& League);
    static void UpdateChemistryAfterGame(FTeamState& Team, bool bWon, const FLeagueState& League);
    static void UpdateChemistryAfterTrade(FTeamState& Team, int32 PlayersTraded);
    static void UpdateMoraleAfterGame(FTeamState& Team, bool bWon, bool bCloseGame);
    static int32 GetSimBonus(const FTeamState& Team);
};
