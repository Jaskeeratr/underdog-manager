#include "TradeService.h"
#include "ManagementService.h"
#include "DeterministicRandom.h"

namespace
{
    FTeamState* FindTeam(FLeagueState& League, const FGuid& TeamId)
    {
        return League.Teams.FindByPredicate([&TeamId](const FTeamState& Team) { return Team.TeamId == TeamId; });
    }

    const FTeamState* FindTeamConst(const FLeagueState& League, const FGuid& TeamId)
    {
        return League.Teams.FindByPredicate([&TeamId](const FTeamState& Team) { return Team.TeamId == TeamId; });
    }

    FPlayerProfile* FindPlayerOnTeam(FTeamState& Team, const FGuid& PlayerId)
    {
        return Team.Players.FindByPredicate([&PlayerId](const FPlayerProfile& P) { return P.PlayerId == PlayerId; });
    }

    FAthleteState* FindStateOnTeam(FTeamState& Team, const FGuid& PlayerId)
    {
        return Team.PlayerStates.FindByPredicate([&PlayerId](const FAthleteState& S) { return S.PlayerId == PlayerId; });
    }

    FGuid DeterministicGuid(FDeterministicRandom& Random)
    {
        return FGuid(Random.NextUInt32(), Random.NextUInt32(), Random.NextUInt32(), Random.NextUInt32());
    }
}

int32 FTradeService::CalculatePlayerValue(const FPlayerProfile& Player, const FAthleteState& State)
{
    const int32 Overall = Player.Ratings.Overall();
    const int32 AgeFactor = FMath::Max(0, 34 - Player.Age);
    const int32 PotentialUpside = FMath::Max(0, Player.Ratings.Potential - Overall);
    const int32 FormBonus = (State.RecentForm - 50) / 5;
    const int32 InjuryPenalty = State.InjuryGamesRemaining * 8;
    const int32 ContractValue = Player.Contract.YearsRemaining <= 1 ? -5 : 0;
    return Overall * 10 + AgeFactor * 3 + PotentialUpside * 4 + FormBonus - InjuryPenalty + ContractValue;
}

bool FTradeService::ProposeTrade(FLeagueState& League, const FGuid& ProposingTeamId,
    const TArray<FGuid>& OutgoingPlayerIds, const FGuid& ReceivingTeamId,
    const TArray<FGuid>& IncomingPlayerIds, FString& OutError)
{
    if (League.Phase != ESeasonPhase::RegularSeason)
    {
        OutError = TEXT("Trades are only permitted during the regular season.");
        return false;
    }
    if (League.CurrentRound >= League.TradeDeadlineRound)
    {
        OutError = TEXT("The trade deadline has passed.");
        return false;
    }
    if (ProposingTeamId == ReceivingTeamId)
    {
        OutError = TEXT("A team cannot trade with itself.");
        return false;
    }
    if (OutgoingPlayerIds.Num() == 0 || IncomingPlayerIds.Num() == 0)
    {
        OutError = TEXT("Both sides of a trade must include at least one player.");
        return false;
    }
    const FTeamState* Proposer = FindTeamConst(League, ProposingTeamId);
    const FTeamState* Receiver = FindTeamConst(League, ReceivingTeamId);
    if (!Proposer || !Receiver)
    {
        OutError = TEXT("One or both teams do not exist.");
        return false;
    }
    for (const FGuid& Pid : OutgoingPlayerIds)
    {
        if (!Proposer->Players.FindByPredicate([&Pid](const FPlayerProfile& P) { return P.PlayerId == Pid; }))
        {
            OutError = TEXT("An outgoing player does not belong to the proposing team.");
            return false;
        }
    }
    for (const FGuid& Pid : IncomingPlayerIds)
    {
        if (!Receiver->Players.FindByPredicate([&Pid](const FPlayerProfile& P) { return P.PlayerId == Pid; }))
        {
            OutError = TEXT("An incoming player does not belong to the receiving team.");
            return false;
        }
    }
    if (Proposer->Players.Num() - OutgoingPlayerIds.Num() + IncomingPlayerIds.Num() < 10
        || Receiver->Players.Num() - IncomingPlayerIds.Num() + OutgoingPlayerIds.Num() < 10)
    {
        OutError = TEXT("Both teams must retain at least 10 players after a trade.");
        return false;
    }

    const uint64 Seed = static_cast<uint64>(League.LeagueSeed) ^ GetTypeHash(ProposingTeamId)
        ^ GetTypeHash(ReceivingTeamId) ^ (static_cast<uint64>(League.CurrentRound) << 32)
        ^ 0x54524144ULL;
    FDeterministicRandom Random(Seed);

    FTradeOffer Offer;
    Offer.TradeId = DeterministicGuid(Random);
    Offer.ProposingTeamId = ProposingTeamId;
    Offer.ReceivingTeamId = ReceivingTeamId;
    Offer.ProposedRound = League.CurrentRound;
    for (const FGuid& Pid : OutgoingPlayerIds)
    {
        FTradeAsset Asset;
        Asset.PlayerId = Pid;
        Asset.FromTeamId = ProposingTeamId;
        Offer.Outgoing.Add(Asset);
    }
    for (const FGuid& Pid : IncomingPlayerIds)
    {
        FTradeAsset Asset;
        Asset.PlayerId = Pid;
        Asset.FromTeamId = ReceivingTeamId;
        Offer.Incoming.Add(Asset);
    }

    if (EvaluateTrade(League, Offer))
    {
        Offer.Status = ETradeStatus::Accepted;
        League.TradeHistory.Add(Offer);
        return ExecuteTrade(League, Offer.TradeId, OutError);
    }

    Offer.Status = ETradeStatus::Rejected;
    League.TradeHistory.Add(Offer);
    OutError = TEXT("The opposing front office has declined this trade proposal.");
    return false;
}

bool FTradeService::EvaluateTrade(const FLeagueState& League, const FTradeOffer& Offer)
{
    const FTeamState* Receiver = FindTeamConst(League, Offer.ReceivingTeamId);
    if (!Receiver) { return false; }

    int32 IncomingValue = 0;
    int32 OutgoingValue = 0;

    for (const FTradeAsset& Asset : Offer.Outgoing)
    {
        const FTeamState* Team = FindTeamConst(League, Asset.FromTeamId);
        if (!Team) { return false; }
        const FPlayerProfile* Player = Team->Players.FindByPredicate(
            [&Asset](const FPlayerProfile& P) { return P.PlayerId == Asset.PlayerId; });
        const FAthleteState* State = Team->PlayerStates.FindByPredicate(
            [&Asset](const FAthleteState& S) { return S.PlayerId == Asset.PlayerId; });
        if (!Player || !State) { return false; }
        IncomingValue += CalculatePlayerValue(*Player, *State);
    }

    for (const FTradeAsset& Asset : Offer.Incoming)
    {
        const FTeamState* Team = FindTeamConst(League, Asset.FromTeamId);
        if (!Team) { return false; }
        const FPlayerProfile* Player = Team->Players.FindByPredicate(
            [&Asset](const FPlayerProfile& P) { return P.PlayerId == Asset.PlayerId; });
        const FAthleteState* State = Team->PlayerStates.FindByPredicate(
            [&Asset](const FAthleteState& S) { return S.PlayerId == Asset.PlayerId; });
        if (!Player || !State) { return false; }
        OutgoingValue += CalculatePlayerValue(*Player, *State);
    }

    return IncomingValue >= static_cast<int32>(OutgoingValue * 0.85);
}

bool FTradeService::ExecuteTrade(FLeagueState& League, const FGuid& TradeId, FString& OutError)
{
    FTradeOffer* Offer = League.TradeHistory.FindByPredicate(
        [&TradeId](const FTradeOffer& O) { return O.TradeId == TradeId; });
    if (!Offer || Offer->Status != ETradeStatus::Accepted)
    {
        OutError = TEXT("Trade not found or not in accepted state.");
        return false;
    }

    FTeamState* Proposer = FindTeam(League, Offer->ProposingTeamId);
    FTeamState* Receiver = FindTeam(League, Offer->ReceivingTeamId);
    if (!Proposer || !Receiver)
    {
        OutError = TEXT("Trade teams no longer exist.");
        return false;
    }

    TArray<FPlayerProfile> MovingToReceiver;
    TArray<FAthleteState> StatesMovingToReceiver;
    for (const FTradeAsset& Asset : Offer->Outgoing)
    {
        const int32 PlayerIdx = Proposer->Players.IndexOfByPredicate(
            [&Asset](const FPlayerProfile& P) { return P.PlayerId == Asset.PlayerId; });
        const int32 StateIdx = Proposer->PlayerStates.IndexOfByPredicate(
            [&Asset](const FAthleteState& S) { return S.PlayerId == Asset.PlayerId; });
        if (PlayerIdx == INDEX_NONE || StateIdx == INDEX_NONE)
        {
            OutError = TEXT("An outgoing player could not be found on the proposing team.");
            return false;
        }
        MovingToReceiver.Add(Proposer->Players[PlayerIdx]);
        StatesMovingToReceiver.Add(Proposer->PlayerStates[StateIdx]);
        Proposer->Players.RemoveAt(PlayerIdx);
        Proposer->PlayerStates.RemoveAt(StateIdx);
    }

    TArray<FPlayerProfile> MovingToProposer;
    TArray<FAthleteState> StatesMovingToProposer;
    for (const FTradeAsset& Asset : Offer->Incoming)
    {
        const int32 PlayerIdx = Receiver->Players.IndexOfByPredicate(
            [&Asset](const FPlayerProfile& P) { return P.PlayerId == Asset.PlayerId; });
        const int32 StateIdx = Receiver->PlayerStates.IndexOfByPredicate(
            [&Asset](const FAthleteState& S) { return S.PlayerId == Asset.PlayerId; });
        if (PlayerIdx == INDEX_NONE || StateIdx == INDEX_NONE)
        {
            OutError = TEXT("An incoming player could not be found on the receiving team.");
            return false;
        }
        MovingToProposer.Add(Receiver->Players[PlayerIdx]);
        StatesMovingToProposer.Add(Receiver->PlayerStates[StateIdx]);
        Receiver->Players.RemoveAt(PlayerIdx);
        Receiver->PlayerStates.RemoveAt(StateIdx);
    }

    Proposer->Players.Append(MovingToProposer);
    Proposer->PlayerStates.Append(StatesMovingToProposer);
    Receiver->Players.Append(MovingToReceiver);
    Receiver->PlayerStates.Append(StatesMovingToReceiver);

    FManagementService::AutoBuildRotation(League, Proposer->TeamId, OutError);
    FManagementService::AutoBuildRotation(League, Receiver->TeamId, OutError);
    return true;
}

void FTradeService::ExpireDeadlineTrades(FLeagueState& League)
{
    for (FTradeOffer& Offer : League.TradeHistory)
    {
        if (Offer.Status == ETradeStatus::Pending) { Offer.Status = ETradeStatus::Expired; }
    }
}
