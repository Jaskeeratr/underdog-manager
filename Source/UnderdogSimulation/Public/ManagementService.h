#pragma once

#include "CoreMinimal.h"
#include "UnderdogCoreTypes.h"

class UNDERDOGSIMULATION_API FManagementService
{
public:
    static bool AutoBuildRotation(FLeagueState& League, const FGuid& TeamId, FString& OutError);
    static bool SetTrainingPlan(FLeagueState& League, const FGuid& TeamId,
        ETrainingFocus Focus, ETrainingIntensity Intensity, FString& OutError);
    static bool AssignScout(FLeagueState& League, const FGuid& RequestingTeamId,
        const FGuid& PlayerId, FString& OutError);
    static void ProcessRound(FLeagueState& League);

private:
    static void ProcessTraining(FLeagueState& League);
    static void ProcessScouting(FLeagueState& League);
};
