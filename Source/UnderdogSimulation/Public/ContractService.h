#pragma once

#include "CoreMinimal.h"
#include "UnderdogCoreTypes.h"

class UNDERDOGSIMULATION_API FContractService
{
public:
    static TArray<FExtensionOffer> GetEligibleExtensions(const FLeagueState& League, const FGuid& TeamId);
    static bool OfferExtension(FLeagueState& League, const FGuid& TeamId, const FGuid& PlayerId,
        int64 OfferedSalary, int32 OfferedYears, FString& OutError);
    static FExtensionOffer CalculateAskingPrice(const FPlayerProfile& Player, const FAthleteState* State);
};
