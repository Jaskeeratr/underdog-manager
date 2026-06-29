#include "FreeAgencyService.h"
#include "ManagementService.h"
#include "DeterministicRandom.h"

int64 FFreeAgencyService::CalculateAskingPrice(const FPlayerProfile& Player)
{
    const int32 Overall = Player.Ratings.Overall();
    int64 Base = static_cast<int64>(Overall) * 2500000LL;
    if (Overall >= 80) { Base += 5000000000LL; }
    else if (Overall >= 70) { Base += 2000000000LL; }
    if (Player.Age <= 25) { Base += 1500000000LL; }
    else if (Player.Age >= 33) { Base = Base * 7 / 10; }
    return FMath::Max(Base, 80000000LL);
}

bool FFreeAgencyService::CanAfford(const FTeamState& Team, int64 Salary)
{
    return Team.TotalSalary() + Salary <= Team.LuxuryTaxThreshold();
}

void FFreeAgencyService::BuildFreeAgentPool(FLeagueState& League)
{
    League.Offseason.FreeAgentPool.Reset();
    for (FTeamState& Team : League.Teams)
    {
        for (int32 Index = Team.Players.Num() - 1; Index >= 0; --Index)
        {
            FPlayerProfile& Player = Team.Players[Index];
            if (Player.Contract.YearsRemaining > 0) { continue; }

            FFreeAgent FA;
            FA.Profile = Player;
            FA.PreviousTeamId = Team.TeamId;
            FA.AskingSalary = CalculateAskingPrice(Player);

            League.Offseason.FreeAgentPool.Add(FA);

            const FGuid Pid = Player.PlayerId;
            Team.Players.RemoveAt(Index);
            const int32 StateIdx = Team.PlayerStates.IndexOfByPredicate(
                [&Pid](const FAthleteState& S) { return S.PlayerId == Pid; });
            if (StateIdx != INDEX_NONE) { Team.PlayerStates.RemoveAt(StateIdx); }
        }
    }

    League.Offseason.FreeAgentPool.Sort([](const FFreeAgent& A, const FFreeAgent& B)
    {
        return A.Profile.Ratings.Overall() > B.Profile.Ratings.Overall();
    });
}

bool FFreeAgencyService::SignFreeAgent(FLeagueState& League, const FGuid& TeamId,
    int32 FreeAgentIndex, int64 OfferSalary, int32 OfferYears, FString& OutError)
{
    if (FreeAgentIndex < 0 || FreeAgentIndex >= League.Offseason.FreeAgentPool.Num())
    {
        OutError = TEXT("Invalid free agent selection.");
        return false;
    }
    FFreeAgent& FA = League.Offseason.FreeAgentPool[FreeAgentIndex];
    if (FA.bSigned)
    {
        OutError = TEXT("This free agent has already signed.");
        return false;
    }
    FTeamState* Team = League.Teams.FindByPredicate(
        [&TeamId](const FTeamState& T) { return T.TeamId == TeamId; });
    if (!Team)
    {
        OutError = TEXT("Team not found.");
        return false;
    }
    if (Team->Players.Num() >= 15)
    {
        OutError = TEXT("Roster is full (15 players maximum).");
        return false;
    }
    if (OfferSalary < FA.AskingSalary * 85 / 100)
    {
        OutError = TEXT("Offer is too low. The player declined.");
        return false;
    }
    if (!CanAfford(*Team, OfferSalary))
    {
        OutError = TEXT("Signing would exceed the luxury tax threshold.");
        return false;
    }
    if (OfferYears < 1 || OfferYears > 4)
    {
        OutError = TEXT("Contract length must be 1-4 years.");
        return false;
    }

    FA.bSigned = true;
    FA.SignedByTeamId = TeamId;

    FPlayerProfile SignedPlayer = FA.Profile;
    SignedPlayer.Contract.SalaryMinorUnits = OfferSalary;
    SignedPlayer.Contract.YearsRemaining = OfferYears;
    Team->Players.Add(SignedPlayer);

    FAthleteState State;
    State.PlayerId = SignedPlayer.PlayerId;
    State.Morale = 60;
    Team->PlayerStates.Add(State);

    FString RotError;
    FManagementService::AutoBuildRotation(League, TeamId, RotError);
    return true;
}

void FFreeAgencyService::AutoSignFreeAgents(FLeagueState& League)
{
    FDeterministicRandom Random(static_cast<uint64>(League.LeagueSeed)
        ^ (static_cast<uint64>(League.SeasonNumber) << 20) ^ 0x465245454147ULL);

    for (int32 Index = 0; Index < League.Offseason.FreeAgentPool.Num(); ++Index)
    {
        FFreeAgent& FA = League.Offseason.FreeAgentPool[Index];
        if (FA.bSigned) { continue; }
        if (FA.Profile.Ratings.Overall() < 40) { continue; }

        FTeamState* BestTeam = nullptr;
        for (FTeamState& Team : League.Teams)
        {
            if (Team.Players.Num() >= 15) { continue; }
            if (!CanAfford(Team, FA.AskingSalary)) { continue; }

            bool bNeedsPosition = true;
            int32 PosCount = 0;
            for (const FPlayerProfile& P : Team.Players)
            {
                if (P.Position == FA.Profile.Position) { PosCount++; }
            }
            if (PosCount >= 4) { bNeedsPosition = false; }

            if (bNeedsPosition && (!BestTeam || Team.Players.Num() < BestTeam->Players.Num()))
            {
                BestTeam = &Team;
            }
        }

        if (BestTeam)
        {
            FString Error;
            const int32 Years = Random.Range(1, 3);
            SignFreeAgent(League, BestTeam->TeamId, Index, FA.AskingSalary, Years, Error);
        }
    }

    for (FTeamState& Team : League.Teams)
    {
        while (Team.Players.Num() < 10)
        {
            for (int32 Index = League.Offseason.FreeAgentPool.Num() - 1; Index >= 0; --Index)
            {
                FFreeAgent& FA = League.Offseason.FreeAgentPool[Index];
                if (FA.bSigned) { continue; }
                FString Error;
                const int64 MinSalary = 80000000LL;
                FA.AskingSalary = MinSalary;
                SignFreeAgent(League, Team.TeamId, Index, MinSalary, 1, Error);
                break;
            }
            if (League.Offseason.FreeAgentPool.Num() == 0) { break; }
            bool bAnyUnsigned = false;
            for (const FFreeAgent& FA : League.Offseason.FreeAgentPool) { if (!FA.bSigned) { bAnyUnsigned = true; break; } }
            if (!bAnyUnsigned) { break; }
        }
    }
}
