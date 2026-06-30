#include "MatchPresentationService.h"
#include "GameRecapService.h"
#include "HighlightDirectorService.h"
#include "RivalryService.h"

FTeamPresentationData FMatchPresentationService::BuildTeamData(
    const FLeagueState& League, const FTeamState& Team, const FGuid& OpponentId)
{
    FTeamPresentationData Data;
    Data.TeamId = Team.TeamId;
    Data.City = Team.City;
    Data.Nickname = Team.Nickname;
    Data.FullName = Team.City + TEXT(" ") + Team.Nickname;
    Data.Wins = Team.Wins;
    Data.Losses = Team.Losses;
    Data.WinStreak = Team.WinStreak;
    Data.Chemistry = Team.Chemistry;
    Data.Tactics = Team.Tactics;

    for (int32 I = 0; I < Team.Rotation.OrderedPlayers.Num() && Data.StartingFive.Num() < 5; ++I)
    {
        const FGuid& PlayerId = Team.Rotation.OrderedPlayers[I];
        const FPlayerProfile* Player = Team.Players.FindByPredicate(
            [&PlayerId](const FPlayerProfile& P) { return P.PlayerId == PlayerId; });
        const FAthleteState* State = Team.PlayerStates.FindByPredicate(
            [&PlayerId](const FAthleteState& S) { return S.PlayerId == PlayerId; });
        if (Player && (!State || State->InjuryGamesRemaining <= 0))
        {
            Data.StartingFive.Add(*Player);
        }
    }

    for (const FAthleteState& State : Team.PlayerStates)
    {
        if (State.InjuryGamesRemaining > 0)
        {
            const FPlayerProfile* Player = Team.Players.FindByPredicate(
                [&State](const FPlayerProfile& P) { return P.PlayerId == State.PlayerId; });
            if (Player)
            {
                Data.InjuredPlayers.Add(FString::Printf(TEXT("%s (%s, %d games)"),
                    *Player->DisplayName, *State.InjuryDescription, State.InjuryGamesRemaining));
            }
        }
    }

    const FRivalry* Rivalry = FRivalryService::GetRivalry(League, Team.TeamId, OpponentId);
    Data.RivalryIntensity = Rivalry ? Rivalry->Intensity : 0;

    return Data;
}

FString FMatchPresentationService::ComputeStandingsImplication(
    const FLeagueState& League, const FGuid& HomeTeamId, const FGuid& AwayTeamId)
{
    if (League.Phase == ESeasonPhase::Playoffs)
    {
        for (const FPlayoffSeries& Series : League.Playoffs.Series)
        {
            if (Series.bComplete) { continue; }
            if ((Series.HigherSeedTeamId == HomeTeamId && Series.LowerSeedTeamId == AwayTeamId) ||
                (Series.HigherSeedTeamId == AwayTeamId && Series.LowerSeedTeamId == HomeTeamId))
            {
                if (Series.HigherSeedWins == 3 || Series.LowerSeedWins == 3)
                {
                    return TEXT("Elimination game — one team can close out the series!");
                }
                return FString::Printf(TEXT("Playoff series: %d-%d"),
                    Series.HigherSeedWins, Series.LowerSeedWins);
            }
        }
        return TEXT("Playoff game");
    }

    const FTeamState* Home = League.Teams.FindByPredicate(
        [&HomeTeamId](const FTeamState& T) { return T.TeamId == HomeTeamId; });
    const FTeamState* Away = League.Teams.FindByPredicate(
        [&AwayTeamId](const FTeamState& T) { return T.TeamId == AwayTeamId; });
    if (!Home || !Away) { return FString(); }

    const int32 WinDiff = FMath::Abs(Home->Wins - Away->Wins);
    if (WinDiff <= 2 && League.CurrentRound >= 15)
    {
        return TEXT("Playoff positioning on the line — every game counts!");
    }
    if (Home->WinStreak >= 5) { return FString::Printf(TEXT("%s riding a %d-game win streak"), *Home->Nickname, Home->WinStreak); }
    if (Away->WinStreak >= 5) { return FString::Printf(TEXT("%s riding a %d-game win streak"), *Away->Nickname, Away->WinStreak); }

    return FString();
}

FMatchPresentationPackage FMatchPresentationService::BuildPackage(
    const FLeagueState& League, const FMatchResult& Result,
    const FMatchSnapshot& Snapshot, const FGameRecap& Recap)
{
    FMatchPresentationPackage Package;
    Package.GameId = Result.GameId;
    Package.Result = Result;
    Package.Recap = Recap;

    const FTeamState* HomeTeam = League.Teams.FindByPredicate(
        [&Snapshot](const FTeamState& T) { return T.TeamId == Snapshot.HomeTeam.TeamId; });
    const FTeamState* AwayTeam = League.Teams.FindByPredicate(
        [&Snapshot](const FTeamState& T) { return T.TeamId == Snapshot.AwayTeam.TeamId; });

    if (HomeTeam) { Package.Home = BuildTeamData(League, *HomeTeam, Snapshot.AwayTeam.TeamId); }
    if (AwayTeam) { Package.Away = BuildTeamData(League, *AwayTeam, Snapshot.HomeTeam.TeamId); }

    Package.Highlights = FHighlightDirectorService::BuildHighlights(Result, Snapshot);

    const FRivalry* Rivalry = FRivalryService::GetRivalry(League,
        Snapshot.HomeTeam.TeamId, Snapshot.AwayTeam.TeamId);
    Package.RivalryIntensity = Rivalry ? Rivalry->Intensity : 0;

    const FScheduledGame* Game = League.Schedule.FindByPredicate(
        [&Result](const FScheduledGame& G) { return G.GameId == Result.GameId; });
    Package.bPlayoffGame = Game && Game->Round >= 22;

    Package.StandingsImplication = ComputeStandingsImplication(League,
        Snapshot.HomeTeam.TeamId, Snapshot.AwayTeam.TeamId);

    return Package;
}
