#pragma once

#include "CoreMinimal.h"
#include "UnderdogCoreTypes.h"

class UNDERDOGSIMULATION_API FMatchPresentationService
{
public:
    static FMatchPresentationPackage BuildPackage(const FLeagueState& League,
        const FMatchResult& Result, const FMatchSnapshot& Snapshot, const FGameRecap& Recap);
    static FTeamPresentationData BuildTeamData(const FLeagueState& League, const FTeamState& Team,
        const FGuid& OpponentId);
    static FString ComputeStandingsImplication(const FLeagueState& League,
        const FGuid& HomeTeamId, const FGuid& AwayTeamId);
};
