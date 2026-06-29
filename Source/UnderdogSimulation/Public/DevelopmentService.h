#pragma once

#include "CoreMinimal.h"
#include "UnderdogCoreTypes.h"

class UNDERDOGSIMULATION_API FDevelopmentService
{
public:
    static void EstablishMentorships(FLeagueState& League);
    static void ProcessDevelopment(FLeagueState& League);
    static void ApplyBreakoutBust(FLeagueState& League);
};
