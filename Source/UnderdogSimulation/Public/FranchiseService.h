#pragma once

#include "CoreMinimal.h"
#include "UnderdogCoreTypes.h"

class UNDERDOGSIMULATION_API FFranchiseService
{
public:
    static void InitializeTeam(FTeamState& Team, int32 MarketIndex = 0);
    static void ProcessCompletedGame(FLeagueState& League, const FMatchResult& Result);
    static void ResetForNewSeason(FLeagueState& League);
    static bool SetTicketPrice(FLeagueState& League, const FGuid& TeamId,
        int64 TicketPriceMinorUnits, FString& OutError);
    static bool UpgradeFacility(FLeagueState& League, const FGuid& TeamId,
        EFacilityType Type, FString& OutError);
    static int64 GetFacilityUpgradeCost(const FTeamState& Team, EFacilityType Type);
    static int32 GetFacilityLevel(const FTeamState& Team, EFacilityType Type);
    static int64 ProjectNextHomeGameRevenue(const FLeagueState& League,
        const FGuid& TeamId, const FGuid& OpponentId);
    static void EvaluateOwnership(FTeamState& Team, ESeasonPhase Phase);
};
