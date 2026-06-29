#pragma once

#include "CoreMinimal.h"
#include "UnderdogCoreTypes.h"

class UNDERDOGSIMULATION_API FTradeService
{
public:
    static bool ProposeTrade(FLeagueState& League, const FGuid& ProposingTeamId,
        const TArray<FGuid>& OutgoingPlayerIds, const FGuid& ReceivingTeamId,
        const TArray<FGuid>& IncomingPlayerIds, FString& OutError);
    static bool EvaluateTrade(const FLeagueState& League, const FTradeOffer& Offer);
    static bool ExecuteTrade(FLeagueState& League, const FGuid& TradeId, FString& OutError);
    static int32 CalculatePlayerValue(const FPlayerProfile& Player, const FAthleteState& State);
    static void ExpireDeadlineTrades(FLeagueState& League);
};
