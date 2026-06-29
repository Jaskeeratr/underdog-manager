#pragma once

#include "CoreMinimal.h"
#include "UnderdogCoreTypes.h"

class UNDERDOGSIMULATION_API FAIManagerService
{
public:
    static void ProcessRound(FLeagueState& League, const FGuid& PlayerTeamId);

private:
    static void AdjustRotation(FLeagueState& League, FTeamState& Team);
    static void ChooseTraining(FTeamState& Team);
    static void AssignScouts(FLeagueState& League, FTeamState& Team);
    static void ConsiderTrades(FLeagueState& League, FTeamState& Team);
};
