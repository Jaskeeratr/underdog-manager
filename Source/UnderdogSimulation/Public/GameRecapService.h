#pragma once

#include "CoreMinimal.h"
#include "UnderdogCoreTypes.h"

class UNDERDOGSIMULATION_API FGameRecapService
{
public:
    static FGameRecap BuildRecap(const FMatchResult& Result, const FMatchSnapshot& Snapshot);
};
