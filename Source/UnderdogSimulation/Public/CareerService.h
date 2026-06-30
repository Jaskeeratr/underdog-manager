#pragma once

#include "CoreMinimal.h"
#include "UnderdogCoreTypes.h"

class UNDERDOGSIMULATION_API FCareerService
{
public:
    static void InitializeCareer(FLeagueState& League, const FString& ManagerName = TEXT("J. Rai"));
    static void RecordGame(FLeagueState& League, const FMatchResult& Result);
    static void EvaluateSeason(FLeagueState& League);
    static bool AcceptJobOffer(FLeagueState& League, const FGuid& TeamId, FString& OutError);
    static bool SetManagerName(FLeagueState& League, const FString& ManagerName, FString& OutError);
    static void GenerateJobOffers(FLeagueState& League);
};
