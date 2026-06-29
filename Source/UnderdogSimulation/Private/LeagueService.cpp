#include "LeagueService.h"
#include "DeterministicRandom.h"
#include "ManagementService.h"
#include "MatchSimulator.h"
#include "AIManagerService.h"
#include "AwardsService.h"
#include "TradeService.h"

FGuid FLeagueService::PlayerTeamId;

bool FLeagueService::BuildSnapshot(
    const FLeagueState& League, const FScheduledGame& Game, FMatchSnapshot& OutSnapshot, FString& OutError)
{
    const FTeamState* Home = League.Teams.FindByPredicate(
        [&Game](const FTeamState& Team) { return Team.TeamId == Game.HomeTeamId; });
    const FTeamState* Away = League.Teams.FindByPredicate(
        [&Game](const FTeamState& Team) { return Team.TeamId == Game.AwayTeamId; });
    if (!Home || !Away)
    {
        OutError = TEXT("The scheduled teams could not be resolved.");
        return false;
    }
    auto HealthyCount = [](const FTeamState& Team)
    {
        int32 Count = 0;
        for (const FAthleteState& State : Team.PlayerStates) { Count += State.InjuryGamesRemaining <= 0 ? 1 : 0; }
        return Count;
    };
    if (HealthyCount(*Home) < 5 || HealthyCount(*Away) < 5)
    {
        OutError = TEXT("Both teams require at least five available athletes.");
        return false;
    }
    OutSnapshot.GameId = Game.GameId;
    OutSnapshot.SimulationVersion = League.SimulationVersion;
    OutSnapshot.Seed = static_cast<int64>(GetTypeHash(Game.GameId) ^ static_cast<uint64>(League.LeagueSeed));
    OutSnapshot.HomeTeam = *Home;
    OutSnapshot.AwayTeam = *Away;
    return true;
}

void FLeagueService::ApplyPlayerConsequences(
    FTeamState& Team, const TArray<FPlayerBoxScore>& BoxScore, bool bWon, uint64 Seed)
{
    FDeterministicRandom Random(Seed);
    for (FAthleteState& State : Team.PlayerStates)
    {
        if (State.InjuryGamesRemaining > 0)
        {
            State.InjuryGamesRemaining--;
            State.Fatigue = FMath::Max(0, State.Fatigue - 14);
            State.Fitness = 100 - State.Fatigue;
            continue;
        }
        const int32 Minutes = Team.Rotation.TargetMinutes.FindRef(State.PlayerId);
        const FPlayerBoxScore* Box = BoxScore.FindByPredicate(
            [&State](const FPlayerBoxScore& Candidate) { return Candidate.PlayerId == State.PlayerId; });
        State.Fatigue = FMath::Clamp(State.Fatigue - 4 + Minutes / 5, 0, 100);
        State.Fitness = 100 - State.Fatigue;
        State.Morale = FMath::Clamp(State.Morale + (bWon ? 2 : -1), 0, 100);
        if (Box)
        {
            State.RecentForm = FMath::Clamp(40 + Box->Points + Box->Rebounds * 2 + Box->Assists * 2
                - Box->Turnovers * 3, 0, 100);
        }
        const FPlayerProfile* Profile = Team.Players.FindByPredicate(
            [&State](const FPlayerProfile& Candidate) { return Candidate.PlayerId == State.PlayerId; });
        const int32 Stamina = Profile ? Profile->Ratings.Stamina : 50;
        const int32 InjuryRisk = FMath::Clamp((State.Fatigue * State.Fatigue) / 8 + FMath::Max(0, 60 - Stamina) * 8, 0, 1800);
        if (Minutes > 0 && Random.ChancePerTenThousand(InjuryRisk))
        {
            State.InjuryGamesRemaining = Random.Range(1, State.Fatigue > 75 ? 5 : 3);
        }
    }
}

void FLeagueService::ApplyResult(FLeagueState& League, FScheduledGame& Game, const FMatchResult& Result)
{
    FTeamState* Home = League.Teams.FindByPredicate(
        [&Game](const FTeamState& Team) { return Team.TeamId == Game.HomeTeamId; });
    FTeamState* Away = League.Teams.FindByPredicate(
        [&Game](const FTeamState& Team) { return Team.TeamId == Game.AwayTeamId; });
    check(Home && Away);
    Game.bComplete = true;
    Game.HomeScore = Result.HomeScore;
    Game.AwayScore = Result.AwayScore;
    Home->PointsFor += Result.HomeScore; Home->PointsAgainst += Result.AwayScore;
    Away->PointsFor += Result.AwayScore; Away->PointsAgainst += Result.HomeScore;
    const bool bHomeWon = Result.HomeScore > Result.AwayScore;
    if (bHomeWon) { Home->Wins++; Away->Losses++; } else { Away->Wins++; Home->Losses++; }
    ApplyPlayerConsequences(*Home, Result.HomeBoxScore, bHomeWon, static_cast<uint64>(Result.Seed) ^ 0x484F4D45ULL);
    ApplyPlayerConsequences(*Away, Result.AwayBoxScore, !bHomeWon, static_cast<uint64>(Result.Seed) ^ 0x41574159ULL);
    FAwardsService::AccumulateBoxScore(League, Result, Game.HomeTeamId, Game.AwayTeamId);
}

bool FLeagueService::SimulateGame(FLeagueState& League, const FGuid& GameId, FMatchResult& OutResult, FString& OutError)
{
    FScheduledGame* Game = League.Schedule.FindByPredicate(
        [&GameId](const FScheduledGame& Candidate) { return Candidate.GameId == GameId; });
    if (!Game || Game->bComplete)
    {
        OutError = TEXT("The requested game does not exist or is already complete.");
        return false;
    }
    FMatchSnapshot Snapshot;
    if (!BuildSnapshot(League, *Game, Snapshot, OutError)) { return false; }
    FPossessionMatchSimulator Simulator;
    FMatchResult Candidate = Simulator.Simulate(Snapshot);
    if (!Candidate.Validate(OutError)) { return false; }
    ApplyResult(League, *Game, Candidate);
    OutResult = MoveTemp(Candidate);
    return true;
}

void FLeagueService::SetPlayerTeamId(FLeagueState& League, const FGuid& TeamId)
{
    PlayerTeamId = TeamId;
}

bool FLeagueService::AdvanceCurrentRound(FLeagueState& League, TArray<FMatchResult>& OutResults, FString& OutError)
{
    if (League.Phase != ESeasonPhase::RegularSeason)
    {
        OutError = TEXT("The regular season is not active.");
        return false;
    }
    if (League.CurrentRound < 0 || League.CurrentRound >= 22)
    {
        OutError = TEXT("The regular season is complete.");
        return false;
    }
    TArray<FGuid> GameIds;
    for (const FScheduledGame& Game : League.Schedule)
    {
        if (Game.Round == League.CurrentRound && !Game.bComplete) { GameIds.Add(Game.GameId); }
    }
    if (GameIds.Num() == 0)
    {
        OutError = TEXT("The current round has no remaining games.");
        return false;
    }
    FLeagueState CandidateLeague = League;
    TArray<FMatchResult> CandidateResults;
    for (const FGuid& GameId : GameIds)
    {
        FMatchResult Result;
        if (!SimulateGame(CandidateLeague, GameId, Result, OutError)) { return false; }
        CandidateResults.Add(MoveTemp(Result));
    }
    CandidateLeague.CurrentRound++;
    FManagementService::ProcessRound(CandidateLeague);
    FAIManagerService::ProcessRound(CandidateLeague, PlayerTeamId);
    if (CandidateLeague.CurrentRound == CandidateLeague.TradeDeadlineRound)
    {
        FTradeService::ExpireDeadlineTrades(CandidateLeague);
    }
    if (CandidateLeague.CurrentRound >= 22)
    {
        CandidateLeague.Phase = ESeasonPhase::Playoffs;
        FString PlayoffError;
        GeneratePlayoffBracket(CandidateLeague, PlayoffError);
    }
    League = MoveTemp(CandidateLeague);
    OutResults = MoveTemp(CandidateResults);
    return true;
}

TArray<FTeamState> FLeagueService::GetStandings(const FLeagueState& League)
{
    TArray<FTeamState> Standings = League.Teams;
    Standings.Sort([&League](const FTeamState& A, const FTeamState& B)
    {
        if (A.Wins != B.Wins) { return A.Wins > B.Wins; }
        int32 HeadToHeadA = 0;
        int32 HeadToHeadB = 0;
        for (const FScheduledGame& Game : League.Schedule)
        {
            if (!Game.bComplete) { continue; }
            if (Game.HomeTeamId == A.TeamId && Game.AwayTeamId == B.TeamId)
            {
                HeadToHeadA += Game.HomeScore > Game.AwayScore ? 1 : 0;
                HeadToHeadB += Game.AwayScore > Game.HomeScore ? 1 : 0;
            }
            else if (Game.HomeTeamId == B.TeamId && Game.AwayTeamId == A.TeamId)
            {
                HeadToHeadB += Game.HomeScore > Game.AwayScore ? 1 : 0;
                HeadToHeadA += Game.AwayScore > Game.HomeScore ? 1 : 0;
            }
        }
        if (HeadToHeadA != HeadToHeadB) { return HeadToHeadA > HeadToHeadB; }
        const int32 DifferentialA = A.PointsFor - A.PointsAgainst;
        const int32 DifferentialB = B.PointsFor - B.PointsAgainst;
        if (DifferentialA != DifferentialB) { return DifferentialA > DifferentialB; }
        if (A.PointsFor != B.PointsFor) { return A.PointsFor > B.PointsFor; }
        return A.TeamId.ToString() < B.TeamId.ToString();
    });
    return Standings;
}

bool FLeagueService::GeneratePlayoffBracket(FLeagueState& League, FString& OutError)
{
    const TArray<FTeamState> Standings = GetStandings(League);
    if (Standings.Num() < 8)
    {
        OutError = TEXT("At least eight teams are required for playoffs.");
        return false;
    }

    FDeterministicRandom Random(static_cast<uint64>(League.LeagueSeed) ^ 0x504C594F4646ULL);
    League.Playoffs = FPlayoffBracket();
    League.Playoffs.CurrentPlayoffRound = 0;

    const int32 Matchups[4][2] = { {0, 7}, {3, 4}, {1, 6}, {2, 5} };
    for (int32 Slot = 0; Slot < 4; ++Slot)
    {
        FPlayoffSeries Series;
        Series.SeriesId = FGuid(Random.NextUInt32(), Random.NextUInt32(), Random.NextUInt32(), Random.NextUInt32());
        Series.BracketSlot = Slot;
        Series.PlayoffRound = 0;
        Series.HigherSeedTeamId = Standings[Matchups[Slot][0]].TeamId;
        Series.LowerSeedTeamId = Standings[Matchups[Slot][1]].TeamId;
        League.Playoffs.Series.Add(Series);
    }
    return true;
}

bool FLeagueService::AdvancePlayoffs(FLeagueState& League, TArray<FMatchResult>& OutResults, FString& OutError)
{
    if (League.Phase != ESeasonPhase::Playoffs)
    {
        OutError = TEXT("The league is not in the playoff phase.");
        return false;
    }
    if (League.Playoffs.IsComplete())
    {
        OutError = TEXT("The playoffs are already complete.");
        return false;
    }

    FDeterministicRandom Random(static_cast<uint64>(League.LeagueSeed)
        ^ (static_cast<uint64>(League.Playoffs.CurrentPlayoffRound) << 16)
        ^ 0x504F535453ULL);

    bool bAllCurrentRoundComplete = true;
    for (FPlayoffSeries& Series : League.Playoffs.Series)
    {
        if (Series.PlayoffRound != League.Playoffs.CurrentPlayoffRound || Series.bComplete) { continue; }

        const bool bHigherSeedHome = (Series.GamesPlayed() % 2 == 0);
        const FGuid HomeId = bHigherSeedHome ? Series.HigherSeedTeamId : Series.LowerSeedTeamId;
        const FGuid AwayId = bHigherSeedHome ? Series.LowerSeedTeamId : Series.HigherSeedTeamId;

        FScheduledGame PlayoffGame;
        PlayoffGame.GameId = FGuid(Random.NextUInt32(), Random.NextUInt32(), Random.NextUInt32(), Random.NextUInt32());
        PlayoffGame.HomeTeamId = HomeId;
        PlayoffGame.AwayTeamId = AwayId;
        PlayoffGame.Round = 22 + League.Playoffs.CurrentPlayoffRound;
        League.Schedule.Add(PlayoffGame);

        FMatchResult Result;
        if (!SimulateGame(League, PlayoffGame.GameId, Result, OutError)) { return false; }
        OutResults.Add(Result);

        Series.GameIds.Add(PlayoffGame.GameId);
        if (Result.HomeScore > Result.AwayScore)
        {
            if (HomeId == Series.HigherSeedTeamId) { Series.HigherSeedWins++; }
            else { Series.LowerSeedWins++; }
        }
        else
        {
            if (AwayId == Series.HigherSeedTeamId) { Series.HigherSeedWins++; }
            else { Series.LowerSeedWins++; }
        }

        if (Series.HigherSeedWins >= 4 || Series.LowerSeedWins >= 4)
        {
            Series.bComplete = true;
        }
        else
        {
            bAllCurrentRoundComplete = false;
        }
    }

    bool bStillActive = false;
    for (const FPlayoffSeries& Series : League.Playoffs.Series)
    {
        if (Series.PlayoffRound == League.Playoffs.CurrentPlayoffRound && !Series.bComplete)
        {
            bStillActive = true;
            break;
        }
    }

    if (!bStillActive)
    {
        TArray<FGuid> Winners;
        for (const FPlayoffSeries& Series : League.Playoffs.Series)
        {
            if (Series.PlayoffRound == League.Playoffs.CurrentPlayoffRound)
            {
                Winners.Add(Series.GetWinnerId());
            }
        }

        if (Winners.Num() == 1)
        {
            League.Playoffs.ChampionTeamId = Winners[0];
            League.Phase = ESeasonPhase::Complete;
        }
        else if (Winners.Num() >= 2)
        {
            League.Playoffs.CurrentPlayoffRound++;
            for (int32 Index = 0; Index + 1 < Winners.Num(); Index += 2)
            {
                FPlayoffSeries NextSeries;
                NextSeries.SeriesId = FGuid(Random.NextUInt32(), Random.NextUInt32(),
                    Random.NextUInt32(), Random.NextUInt32());
                NextSeries.BracketSlot = Index / 2;
                NextSeries.PlayoffRound = League.Playoffs.CurrentPlayoffRound;
                NextSeries.HigherSeedTeamId = Winners[Index];
                NextSeries.LowerSeedTeamId = Winners[Index + 1];
                League.Playoffs.Series.Add(NextSeries);
            }
        }
    }

    FManagementService::ProcessRound(League);
    return true;
}
