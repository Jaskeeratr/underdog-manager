#include "TradeService.h"
#include "ChemistryService.h"
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
    const int32 ContractValue = Player.Contract.YearsRemaining >= 2 ? 12 : -8;
    const int32 ExpectedSalaryMillions = FMath::Max(1, Overall * Overall / 350);
    const int32 ActualSalaryMillions = static_cast<int32>(Player.Contract.SalaryMinorUnits / 100000000LL);
    const int32 SalaryPenalty = FMath::Max(0, ActualSalaryMillions - ExpectedSalaryMillions) * 4;
    return Overall * 10 + AgeFactor * 3 + PotentialUpside * 5 + FormBonus
        - InjuryPenalty + ContractValue - SalaryPenalty;
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
    if (OutgoingPlayerIds.Num() > 3 || IncomingPlayerIds.Num() > 3)
    {
        OutError = TEXT("A trade may contain no more than three players per team.");
        return false;
    }
    TSet<FGuid> UniqueAssets;
    for (const FGuid& PlayerId : OutgoingPlayerIds) { UniqueAssets.Add(PlayerId); }
    for (const FGuid& PlayerId : IncomingPlayerIds) { UniqueAssets.Add(PlayerId); }
    if (UniqueAssets.Num() != OutgoingPlayerIds.Num() + IncomingPlayerIds.Num())
    {
        OutError = TEXT("A player cannot appear more than once in a trade.");
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
    if (Proposer->Players.Num() - OutgoingPlayerIds.Num() + IncomingPlayerIds.Num() > 15
        || Receiver->Players.Num() - IncomingPlayerIds.Num() + OutgoingPlayerIds.Num() > 15)
    {
        OutError = TEXT("A trade cannot leave either team above the 15-player roster limit.");
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

    const FTradeEvaluation Evaluation = EvaluateTradeDetailed(League, Offer);
    if (!Evaluation.bLegal)
    {
        OutError = Evaluation.Summary;
        return false;
    }
    if (Evaluation.bAccepted)
    {
        Offer.Status = ETradeStatus::Accepted;
        League.TradeHistory.Add(Offer);
        return ExecuteTrade(League, Offer.TradeId, OutError);
    }

    Offer.Status = ETradeStatus::Rejected;
    League.TradeHistory.Add(Offer);
    OutError = Evaluation.Summary;
    return false;
}

bool FTradeService::EvaluateTrade(const FLeagueState& League, const FTradeOffer& Offer)
{
    return EvaluateTradeDetailed(League, Offer).bAccepted;
}

FTradeEvaluation FTradeService::EvaluateTradeDetailed(const FLeagueState& League, const FTradeOffer& Offer)
{
    FTradeEvaluation Evaluation;
    const FTeamState* Proposer = FindTeamConst(League, Offer.ProposingTeamId);
    const FTeamState* Receiver = FindTeamConst(League, Offer.ReceivingTeamId);
    if (!Proposer || !Receiver)
    {
        Evaluation.Summary = TEXT("Trade teams could not be resolved.");
        return Evaluation;
    }
    if (Offer.Outgoing.IsEmpty() || Offer.Incoming.IsEmpty()
        || Offer.Outgoing.Num() > 3 || Offer.Incoming.Num() > 3)
    {
        Evaluation.Summary = TEXT("Trades require one to three players from each team.");
        return Evaluation;
    }

    int64 SalaryToReceiver = 0;
    int64 SalaryToProposer = 0;

    for (const FTradeAsset& Asset : Offer.Outgoing)
    {
        const FTeamState* Team = FindTeamConst(League, Asset.FromTeamId);
        if (!Team) { Evaluation.Summary = TEXT("An offered team no longer exists."); return Evaluation; }
        const FPlayerProfile* Player = Team->Players.FindByPredicate(
            [&Asset](const FPlayerProfile& P) { return P.PlayerId == Asset.PlayerId; });
        const FAthleteState* State = Team->PlayerStates.FindByPredicate(
            [&Asset](const FAthleteState& S) { return S.PlayerId == Asset.PlayerId; });
        if (!Player || !State) { Evaluation.Summary = TEXT("An offered player is unavailable."); return Evaluation; }
        Evaluation.ReceiverReceivesValue += CalculatePlayerValue(*Player, *State);
        SalaryToReceiver += Player->Contract.SalaryMinorUnits;
    }

    for (const FTradeAsset& Asset : Offer.Incoming)
    {
        const FTeamState* Team = FindTeamConst(League, Asset.FromTeamId);
        if (!Team) { Evaluation.Summary = TEXT("A requested team no longer exists."); return Evaluation; }
        const FPlayerProfile* Player = Team->Players.FindByPredicate(
            [&Asset](const FPlayerProfile& P) { return P.PlayerId == Asset.PlayerId; });
        const FAthleteState* State = Team->PlayerStates.FindByPredicate(
            [&Asset](const FAthleteState& S) { return S.PlayerId == Asset.PlayerId; });
        if (!Player || !State) { Evaluation.Summary = TEXT("A requested player is unavailable."); return Evaluation; }
        Evaluation.ProposerReceivesValue += CalculatePlayerValue(*Player, *State);
        SalaryToProposer += Player->Contract.SalaryMinorUnits;
    }

    Evaluation.ProposerSalaryChange = SalaryToProposer - SalaryToReceiver;
    Evaluation.ReceiverSalaryChange = SalaryToReceiver - SalaryToProposer;
    const int64 ProposerPayrollAfter = Proposer->TotalSalary() + Evaluation.ProposerSalaryChange;
    const int64 ReceiverPayrollAfter = Receiver->TotalSalary() + Evaluation.ReceiverSalaryChange;
    const int64 SalaryTolerance = 1000000000LL;
    if ((ProposerPayrollAfter > Proposer->SalaryCapMinorUnits
            && SalaryToProposer > SalaryToReceiver * 125 / 100 + SalaryTolerance)
        || (ReceiverPayrollAfter > Receiver->SalaryCapMinorUnits
            && SalaryToReceiver > SalaryToProposer * 125 / 100 + SalaryTolerance))
    {
        Evaluation.Summary = TEXT("Trade rejected: salary matching fails for a team operating above the cap.");
        Evaluation.Reasons.Add(TEXT("Incoming salary may not exceed 125% of outgoing salary plus the allowed exception."));
        return Evaluation;
    }

    Evaluation.bLegal = true;
    const int32 RequiredValue = Evaluation.ProposerReceivesValue * 95 / 100;
    Evaluation.bAccepted = Evaluation.ReceiverReceivesValue >= RequiredValue;
    const int32 Difference = Evaluation.ReceiverReceivesValue - RequiredValue;
    if (Evaluation.bAccepted)
    {
        Evaluation.Summary = FString::Printf(TEXT("ACCEPTABLE: opposing team gains %d value above its minimum."), Difference);
        Evaluation.Reasons.Add(TEXT("Roster and salary rules pass."));
        Evaluation.Reasons.Add(TEXT("The opposing team receives sufficient age, contract, health, and potential-adjusted value."));
    }
    else
    {
        Evaluation.Summary = FString::Printf(TEXT("DECLINED: add approximately %d trade-value points."), -Difference);
        Evaluation.Reasons.Add(TEXT("The opposing team values the requested players more highly than the offered package."));
    }
    return Evaluation;
}

bool FTradeService::ExecuteTrade(FLeagueState& League, const FGuid& TradeId, FString& OutError)
{
    FLeagueState CandidateLeague = League;
    FTradeOffer* Offer = CandidateLeague.TradeHistory.FindByPredicate(
        [&TradeId](const FTradeOffer& O) { return O.TradeId == TradeId; });
    if (!Offer || Offer->Status != ETradeStatus::Accepted)
    {
        OutError = TEXT("Trade not found or not in accepted state.");
        return false;
    }

    FTeamState* Proposer = FindTeam(CandidateLeague, Offer->ProposingTeamId);
    FTeamState* Receiver = FindTeam(CandidateLeague, Offer->ReceivingTeamId);
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

    if (Proposer->Players.Num() < 10 || Proposer->Players.Num() > 15
        || Receiver->Players.Num() < 10 || Receiver->Players.Num() > 15)
    {
        OutError = TEXT("Trade execution would create an illegal roster size.");
        return false;
    }
    if (!FManagementService::AutoBuildRotation(CandidateLeague, Proposer->TeamId, OutError)
        || !FManagementService::AutoBuildRotation(CandidateLeague, Receiver->TeamId, OutError))
    {
        return false;
    }
    FChemistryService::UpdateChemistryAfterTrade(*Proposer, Offer->Outgoing.Num());
    FChemistryService::UpdateChemistryAfterTrade(*Receiver, Offer->Incoming.Num());
    League = MoveTemp(CandidateLeague);
    return true;
}

void FTradeService::ExpireDeadlineTrades(FLeagueState& League)
{
    for (FTradeOffer& Offer : League.TradeHistory)
    {
        if (Offer.Status == ETradeStatus::Pending) { Offer.Status = ETradeStatus::Expired; }
    }
}
