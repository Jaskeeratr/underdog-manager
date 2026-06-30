#include "LeagueHistoryService.h"

void FLeagueHistoryService::RecordSeasonEnd(FLeagueState& League)
{
    FChampionshipRecord Record;
    Record.Season = League.SeasonNumber;

    if (League.Playoffs.ChampionTeamId.IsValid())
    {
        Record.ChampionTeamId = League.Playoffs.ChampionTeamId;
        const FTeamState* Champ = League.Teams.FindByPredicate(
            [&](const FTeamState& T) { return T.TeamId == League.Playoffs.ChampionTeamId; });
        Record.ChampionName = Champ ? Champ->City + TEXT(" ") + Champ->Nickname : TEXT("Unknown");

        for (const FSeasonAward& Award : League.Awards)
        {
            if (Award.Type == EAwardType::ChampionMVP && Award.Season == League.SeasonNumber)
            {
                Record.MvpPlayerId = Award.PlayerId;
                for (const FTeamState& Team : League.Teams)
                {
                    for (const FPlayerProfile& P : Team.Players)
                    {
                        if (P.PlayerId == Award.PlayerId) { Record.MvpName = P.DisplayName; break; }
                    }
                    if (!Record.MvpName.IsEmpty()) { break; }
                }
                break;
            }
        }
    }
    League.History.Championships.Add(Record);

    League.History.AllAwards.Append(League.Awards.FilterByPredicate(
        [&](const FSeasonAward& A) { return A.Season == League.SeasonNumber; }));

    AccumulatePlayerStats(League);
}

void FLeagueHistoryService::AccumulatePlayerStats(FLeagueState& League)
{
    for (const FSeasonStats& Stats : League.SeasonStats)
    {
        FAllTimeLeader* Existing = League.History.AllTimeLeaders.FindByPredicate(
            [&](const FAllTimeLeader& L) { return L.PlayerId == Stats.PlayerId; });

        if (!Existing)
        {
            FAllTimeLeader NewLeader;
            NewLeader.PlayerId = Stats.PlayerId;

            for (const FTeamState& Team : League.Teams)
            {
                for (const FPlayerProfile& P : Team.Players)
                {
                    if (P.PlayerId == Stats.PlayerId) { NewLeader.PlayerName = P.DisplayName; break; }
                }
                if (!NewLeader.PlayerName.IsEmpty()) { break; }
            }

            League.History.AllTimeLeaders.Add(NewLeader);
            Existing = &League.History.AllTimeLeaders.Last();
        }

        Existing->TotalPoints += Stats.TotalPoints;
        Existing->TotalRebounds += Stats.TotalRebounds;
        Existing->TotalAssists += Stats.TotalAssists;
        Existing->GamesPlayed += Stats.GamesPlayed;
        Existing->SeasonsPlayed++;
    }

    if (League.Playoffs.ChampionTeamId.IsValid())
    {
        const FTeamState* Champ = League.Teams.FindByPredicate(
            [&](const FTeamState& T) { return T.TeamId == League.Playoffs.ChampionTeamId; });
        if (Champ)
        {
            for (const FPlayerProfile& P : Champ->Players)
            {
                FAllTimeLeader* Leader = League.History.AllTimeLeaders.FindByPredicate(
                    [&](const FAllTimeLeader& L) { return L.PlayerId == P.PlayerId; });
                if (Leader) { Leader->Championships++; }
            }
        }
    }

    for (const FSeasonAward& Award : League.Awards)
    {
        if (Award.Season == League.SeasonNumber && Award.Type == EAwardType::MVP)
        {
            FAllTimeLeader* Leader = League.History.AllTimeLeaders.FindByPredicate(
                [&](const FAllTimeLeader& L) { return L.PlayerId == Award.PlayerId; });
            if (Leader) { Leader->MvpAwards++; }
        }
    }
}

TArray<FAllTimeLeader> FLeagueHistoryService::GetTopScorers(const FLeagueHistory& History, int32 Count)
{
    TArray<FAllTimeLeader> Sorted = History.AllTimeLeaders;
    Sorted.Sort([](const FAllTimeLeader& A, const FAllTimeLeader& B) { return A.TotalPoints > B.TotalPoints; });
    if (Sorted.Num() > Count) { Sorted.SetNum(Count); }
    return Sorted;
}

TArray<FAllTimeLeader> FLeagueHistoryService::GetTopRebounders(const FLeagueHistory& History, int32 Count)
{
    TArray<FAllTimeLeader> Sorted = History.AllTimeLeaders;
    Sorted.Sort([](const FAllTimeLeader& A, const FAllTimeLeader& B) { return A.TotalRebounds > B.TotalRebounds; });
    if (Sorted.Num() > Count) { Sorted.SetNum(Count); }
    return Sorted;
}

TArray<FAllTimeLeader> FLeagueHistoryService::GetTopAssisters(const FLeagueHistory& History, int32 Count)
{
    TArray<FAllTimeLeader> Sorted = History.AllTimeLeaders;
    Sorted.Sort([](const FAllTimeLeader& A, const FAllTimeLeader& B) { return A.TotalAssists > B.TotalAssists; });
    if (Sorted.Num() > Count) { Sorted.SetNum(Count); }
    return Sorted;
}
