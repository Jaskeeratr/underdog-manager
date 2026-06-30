#include "CareerService.h"

#include "DeterministicRandom.h"

namespace
{
    FTeamState* FindTeam(FLeagueState& League, const FGuid& TeamId)
    {
        return League.Teams.FindByPredicate(
            [&TeamId](const FTeamState& Team) { return Team.TeamId == TeamId; });
    }

    const FTeamState* FindTeam(const FLeagueState& League, const FGuid& TeamId)
    {
        return League.Teams.FindByPredicate(
            [&TeamId](const FTeamState& Team) { return Team.TeamId == TeamId; });
    }
}

void FCareerService::InitializeCareer(FLeagueState& League, const FString& ManagerName)
{
    League.ManagerCareer = FManagerCareer();
    League.ManagerCareer.ManagerName = ManagerName.TrimStartAndEnd();
    if (!League.Teams.IsEmpty())
    {
        League.ManagerCareer.CurrentTeamId = League.Teams[0].TeamId;
    }
}

void FCareerService::RecordGame(FLeagueState& League, const FMatchResult& Result)
{
    if (League.ManagerCareer.EmploymentStatus != EManagerEmploymentStatus::Employed) { return; }
    const FScheduledGame* Game = League.Schedule.FindByPredicate(
        [&Result](const FScheduledGame& Candidate) { return Candidate.GameId == Result.GameId; });
    if (!Game) { return; }
    const bool bHome = Game->HomeTeamId == League.ManagerCareer.CurrentTeamId;
    const bool bAway = Game->AwayTeamId == League.ManagerCareer.CurrentTeamId;
    if (!bHome && !bAway) { return; }
    const bool bWon = bHome ? Result.HomeScore > Result.AwayScore : Result.AwayScore > Result.HomeScore;
    League.ManagerCareer.CareerWins += bWon ? 1 : 0;
    League.ManagerCareer.CareerLosses += bWon ? 0 : 1;
    League.ManagerCareer.CareerScore = League.ManagerCareer.CareerWins * 3
        + League.ManagerCareer.PlayoffAppearances * 25 + League.ManagerCareer.Championships * 100;
}

void FCareerService::EvaluateSeason(FLeagueState& League)
{
    FManagerCareer& Career = League.ManagerCareer;
    if (Career.EmploymentStatus != EManagerEmploymentStatus::Employed) { GenerateJobOffers(League); return; }
    const FTeamState* Team = FindTeam(League, Career.CurrentTeamId);
    if (!Team) { Career.EmploymentStatus = EManagerEmploymentStatus::Unemployed; GenerateJobOffers(League); return; }

    FManagerSeasonRecord Record;
    Record.Season = League.SeasonNumber;
    Record.TeamId = Team->TeamId;
    Record.TeamName = Team->City + TEXT(" ") + Team->Nickname;
    Record.Wins = Team->Wins;
    Record.Losses = Team->Losses;
    Record.bReachedPlayoffs = League.Playoffs.Series.ContainsByPredicate(
        [Team](const FPlayoffSeries& Series)
        {
            return Series.HigherSeedTeamId == Team->TeamId || Series.LowerSeedTeamId == Team->TeamId;
        });
    Record.bChampion = League.Playoffs.ChampionTeamId == Team->TeamId;
    Record.OwnerConfidence = Team->Franchise.Ownership.Confidence;
    Career.SeasonHistory.Add(Record);
    Career.PlayoffAppearances += Record.bReachedPlayoffs ? 1 : 0;
    Career.Championships += Record.bChampion ? 1 : 0;
    Career.ContractYearsRemaining--;
    Career.CareerScore = Career.CareerWins * 3 + Career.PlayoffAppearances * 25 + Career.Championships * 100;

    if (Record.OwnerConfidence < 25)
    {
        Career.EmploymentStatus = EManagerEmploymentStatus::Unemployed;
        Career.CurrentTeamId.Invalidate();
        Career.ContractYearsRemaining = 0;
    }
    else if (Career.ContractYearsRemaining <= 0)
    {
        Career.ContractYearsRemaining = Record.OwnerConfidence >= 65 ? 3 : 1;
    }
    GenerateJobOffers(League);
}

void FCareerService::GenerateJobOffers(FLeagueState& League)
{
    League.ManagerCareer.JobOffers.Reset();
    TArray<const FTeamState*> Candidates;
    for (const FTeamState& Team : League.Teams)
    {
        if (Team.TeamId == League.ManagerCareer.CurrentTeamId) { continue; }
        Candidates.Add(&Team);
    }
    Candidates.Sort([](const FTeamState& A, const FTeamState& B)
    {
        if (A.Wins != B.Wins) { return A.Wins < B.Wins; }
        return A.Franchise.Ownership.Confidence < B.Franchise.Ownership.Confidence;
    });
    const int32 Reputation = League.ManagerCareer.CareerScore;
    const int32 OfferCount = FMath::Clamp(1 + Reputation / 150, 1, 3);
    for (int32 Index = 0; Index < Candidates.Num() && Index < OfferCount; ++Index)
    {
        FManagerJobOffer Offer;
        Offer.TeamId = Candidates[Index]->TeamId;
        Offer.TeamName = Candidates[Index]->City + TEXT(" ") + Candidates[Index]->Nickname;
        Offer.ContractYears = Reputation >= 250 ? 3 : 2;
        Offer.ExpectedWins = FMath::Clamp(Candidates[Index]->Wins + 3, 6, 14);
        League.ManagerCareer.JobOffers.Add(Offer);
    }
}

bool FCareerService::AcceptJobOffer(FLeagueState& League, const FGuid& TeamId, FString& OutError)
{
    const FManagerJobOffer* Offer = League.ManagerCareer.JobOffers.FindByPredicate(
        [&TeamId](const FManagerJobOffer& Candidate) { return Candidate.TeamId == TeamId; });
    if (!Offer || !FindTeam(League, TeamId)) { OutError = TEXT("That job offer is no longer available."); return false; }
    League.ManagerCareer.CurrentTeamId = TeamId;
    League.ManagerCareer.EmploymentStatus = EManagerEmploymentStatus::Employed;
    League.ManagerCareer.ContractYearsRemaining = Offer->ContractYears;
    League.ManagerCareer.JobOffers.Reset();
    const int32 TeamIndex = League.Teams.IndexOfByPredicate(
        [&TeamId](const FTeamState& Team) { return Team.TeamId == TeamId; });
    if (TeamIndex > 0) { League.Teams.Swap(0, TeamIndex); }
    return true;
}

bool FCareerService::SetManagerName(FLeagueState& League, const FString& ManagerName, FString& OutError)
{
    const FString CleanName = ManagerName.TrimStartAndEnd();
    if (CleanName.Len() < 2 || CleanName.Len() > 32)
    {
        OutError = TEXT("Manager name must contain between 2 and 32 characters.");
        return false;
    }
    League.ManagerCareer.ManagerName = CleanName;
    return true;
}
