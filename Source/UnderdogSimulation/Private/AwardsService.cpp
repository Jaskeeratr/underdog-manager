#include "AwardsService.h"

namespace
{
    FSeasonStats* FindOrAddStats(FLeagueState& League, const FGuid& PlayerId)
    {
        FSeasonStats* Existing = League.SeasonStats.FindByPredicate(
            [&PlayerId](const FSeasonStats& S) { return S.PlayerId == PlayerId; });
        if (Existing) { return Existing; }
        FSeasonStats New;
        New.PlayerId = PlayerId;
        for (const FTeamState& Team : League.Teams)
        {
            const FPlayerProfile* Player = Team.Players.FindByPredicate(
                [&PlayerId](const FPlayerProfile& P) { return P.PlayerId == PlayerId; });
            if (Player) { New.OverallAtSeasonStart = Player->Ratings.Overall(); break; }
        }
        return &League.SeasonStats.Add_GetRef(New);
    }

    void AccumulateTeam(FLeagueState& League, const TArray<FPlayerBoxScore>& BoxScore)
    {
        for (const FPlayerBoxScore& Box : BoxScore)
        {
            FSeasonStats* Stats = FindOrAddStats(League, Box.PlayerId);
            Stats->GamesPlayed++;
            Stats->TotalPoints += Box.Points;
            Stats->TotalRebounds += Box.Rebounds;
            Stats->TotalAssists += Box.Assists;
            Stats->TotalSteals += Box.Steals;
            Stats->TotalBlocks += Box.Blocks;
            Stats->TotalTurnovers += Box.Turnovers;
        }
    }

    const FPlayerProfile* FindPlayerInLeague(const FLeagueState& League, const FGuid& PlayerId, FGuid* OutTeamId = nullptr)
    {
        for (const FTeamState& Team : League.Teams)
        {
            const FPlayerProfile* Player = Team.Players.FindByPredicate(
                [&PlayerId](const FPlayerProfile& P) { return P.PlayerId == PlayerId; });
            if (Player)
            {
                if (OutTeamId) { *OutTeamId = Team.TeamId; }
                return Player;
            }
        }
        return nullptr;
    }
}

void FAwardsService::AccumulateBoxScore(FLeagueState& League, const FMatchResult& Result,
    const FGuid& HomeTeamId, const FGuid& AwayTeamId)
{
    AccumulateTeam(League, Result.HomeBoxScore);
    AccumulateTeam(League, Result.AwayBoxScore);
}

TArray<FSeasonAward> FAwardsService::CalculateAwards(const FLeagueState& League)
{
    TArray<FSeasonAward> Awards;

    const FSeasonStats* MvpCandidate = nullptr;
    float MvpScore = -1.0f;
    const FSeasonStats* DpoyCandidate = nullptr;
    float DpoyScore = -1.0f;
    const FSeasonStats* RoyCandidate = nullptr;
    float RoyScore = -1.0f;
    const FSeasonStats* MipCandidate = nullptr;
    int32 MipImprovement = -1;

    for (const FSeasonStats& Stats : League.SeasonStats)
    {
        if (Stats.GamesPlayed < 10) { continue; }

        FGuid TeamId;
        const FPlayerProfile* Player = FindPlayerInLeague(League, Stats.PlayerId, &TeamId);
        if (!Player) { continue; }

        const float MvpVal = Stats.PPG() + Stats.RPG() * 1.2f + Stats.APG() * 1.5f
            + static_cast<float>(Stats.TotalSteals + Stats.TotalBlocks) / Stats.GamesPlayed * 2.0f
            - static_cast<float>(Stats.TotalTurnovers) / Stats.GamesPlayed * 1.5f;
        if (MvpVal > MvpScore) { MvpScore = MvpVal; MvpCandidate = &Stats; }

        const float DefVal = static_cast<float>(Stats.TotalSteals + Stats.TotalBlocks * 2) / Stats.GamesPlayed
            + Stats.RPG() * 0.5f;
        if (DefVal > DpoyScore) { DpoyScore = DefVal; DpoyCandidate = &Stats; }

        if (Player->Age <= 22)
        {
            if (MvpVal > RoyScore) { RoyScore = MvpVal; RoyCandidate = &Stats; }
        }

        const int32 CurrentOvr = Player->Ratings.Overall();
        const int32 Improvement = CurrentOvr - Stats.OverallAtSeasonStart;
        if (Improvement > MipImprovement) { MipImprovement = Improvement; MipCandidate = &Stats; }
    }

    auto MakeAward = [&League](EAwardType Type, const FSeasonStats* Candidate) -> FSeasonAward
    {
        FSeasonAward Award;
        Award.Type = Type;
        Award.Season = League.SeasonNumber;
        if (Candidate)
        {
            Award.PlayerId = Candidate->PlayerId;
            FGuid TeamId;
            FindPlayerInLeague(League, Candidate->PlayerId, &TeamId);
            Award.TeamId = TeamId;
        }
        return Award;
    };

    if (MvpCandidate) { Awards.Add(MakeAward(EAwardType::MVP, MvpCandidate)); }
    if (DpoyCandidate) { Awards.Add(MakeAward(EAwardType::DPOY, DpoyCandidate)); }
    if (RoyCandidate) { Awards.Add(MakeAward(EAwardType::ROY, RoyCandidate)); }
    if (MipCandidate && MipImprovement > 0) { Awards.Add(MakeAward(EAwardType::MIP, MipCandidate)); }

    return Awards;
}

FSeasonAward FAwardsService::CalculateChampionMVP(const FLeagueState& League)
{
    FSeasonAward Award;
    Award.Type = EAwardType::ChampionMVP;
    Award.Season = League.SeasonNumber;

    if (!League.Playoffs.ChampionTeamId.IsValid()) { return Award; }

    const FSeasonStats* Best = nullptr;
    float BestScore = -1.0f;
    for (const FSeasonStats& Stats : League.SeasonStats)
    {
        FGuid TeamId;
        const FPlayerProfile* Player = FindPlayerInLeague(League, Stats.PlayerId, &TeamId);
        if (!Player || TeamId != League.Playoffs.ChampionTeamId) { continue; }
        const float Score = Stats.PPG() + Stats.RPG() + Stats.APG();
        if (Score > BestScore) { BestScore = Score; Best = &Stats; }
    }
    if (Best)
    {
        Award.PlayerId = Best->PlayerId;
        Award.TeamId = League.Playoffs.ChampionTeamId;
    }
    return Award;
}
