#pragma once

#include "CoreMinimal.h"
#include "UnderdogCoreTypes.h"

class UNDERDOGSIMULATION_API FOffseasonService
{
public:
    static bool StartOffseason(FLeagueState& League, FString& OutError);
    static bool AdvanceOffseason(FLeagueState& League, FString& OutError);
    static bool DraftPlayer(FLeagueState& League, const FGuid& TeamId,
        int32 ProspectIndex, FString& OutError);

private:
    static void ProcessAging(FLeagueState& League);
    static void ProcessContractExpiry(FLeagueState& League);
    static void GenerateDraftClass(FLeagueState& League);
    static void AutoDraft(FLeagueState& League);
    static void ProcessResigning(FLeagueState& League);
    static void ResetForNewSeason(FLeagueState& League);
};
