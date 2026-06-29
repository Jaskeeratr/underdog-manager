#pragma once

#include "CoreMinimal.h"
#include "UnderdogCoreTypes.h"

class UNDERDOGSIMULATION_API FCommentaryService
{
public:
    static TArray<FCommentaryLine> Generate(const FMatchResult& Result, const FMatchSnapshot& Snapshot);
    static TArray<FCommentaryLine> GenerateHighlights(const FMatchResult& Result,
        const FMatchSnapshot& Snapshot, int32 MaxLines = 12);
};
