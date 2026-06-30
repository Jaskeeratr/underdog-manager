#pragma once

#include "CoreMinimal.h"
#include "UnderdogCoreTypes.h"

class UNDERDOGSIMULATION_API FHighlightDirectorService
{
public:
    static TArray<FHighlightCue> BuildHighlights(const FMatchResult& Result,
        const FMatchSnapshot& Snapshot, int32 MaxCues = 10);
    static bool ValidateCues(const TArray<FHighlightCue>& Cues, const FMatchResult& Result, FString& OutError);

private:
    static EHighlightTemplate ClassifyEvent(const FMatchEvent& Event, const FMatchResult& Result);
    static ECameraPreset SelectCamera(EHighlightTemplate Template);
    static float EstimateDuration(EHighlightTemplate Template);
    static int32 ScoreImportance(const FMatchEvent& Event, const FMatchResult& Result);
    static FString FindPlayerName(const FMatchSnapshot& Snapshot, const FGuid& PlayerId);
};
