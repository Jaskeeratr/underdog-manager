#include "LeagueGenerator.h"
#include "DeterministicRandom.h"

namespace
{
    const TCHAR* Cities[] = { TEXT("Calgary"), TEXT("Edmonton"), TEXT("Vancouver"), TEXT("Winnipeg"),
        TEXT("Toronto"), TEXT("Ottawa"), TEXT("Montreal"), TEXT("Halifax"), TEXT("Seattle"),
        TEXT("Portland"), TEXT("Denver"), TEXT("Minneapolis") };
    const TCHAR* Nicknames[] = { TEXT("Chinooks"), TEXT("Aurora"), TEXT("Harbour"), TEXT("Frost"),
        TEXT("Towers"), TEXT("Guard"), TEXT("Royals"), TEXT("Tides"), TEXT("Emeralds"),
        TEXT("Pioneers"), TEXT("Summit"), TEXT("Northstars") };
    const TCHAR* FirstNames[] = { TEXT("Andre"), TEXT("Marcus"), TEXT("Jordan"), TEXT("Darius"), TEXT("Malik"),
        TEXT("Devin"), TEXT("Isaiah"), TEXT("Noah"), TEXT("Eli"), TEXT("Cameron"), TEXT("Jalen"), TEXT("Miles") };
    const TCHAR* LastNames[] = { TEXT("Bennett"), TEXT("Carter"), TEXT("Dawson"), TEXT("Ellis"), TEXT("Foster"),
        TEXT("Grant"), TEXT("Hayes"), TEXT("Irving"), TEXT("James"), TEXT("Knight"), TEXT("Lewis"), TEXT("Mitchell") };

    FGuid DeterministicGuid(FDeterministicRandom& Random)
    {
        return FGuid(Random.NextUInt32(), Random.NextUInt32(), Random.NextUInt32(), Random.NextUInt32());
    }

    EPlayerPosition PositionForIndex(int32 Index)
    {
        return static_cast<EPlayerPosition>(Index % 5);
    }
}

FLeagueState FLeagueGenerator::Generate(uint64 Seed)
{
    FDeterministicRandom Random(Seed);
    FLeagueState League;
    League.LeagueSeed = static_cast<int64>(Seed);
    League.Teams.Reserve(12);

    for (int32 TeamIndex = 0; TeamIndex < 12; ++TeamIndex)
    {
        FTeamState Team;
        Team.TeamId = DeterministicGuid(Random);
        Team.City = Cities[TeamIndex];
        Team.Nickname = Nicknames[TeamIndex];
        Team.OperatingBalanceMinorUnits = 2500000000LL;
        Team.Players.Reserve(15);
        Team.PlayerStates.Reserve(15);

        const int32 TeamQuality = 48 + TeamIndex * 2;
        for (int32 PlayerIndex = 0; PlayerIndex < 15; ++PlayerIndex)
        {
            FPlayerProfile Player;
            Player.PlayerId = DeterministicGuid(Random);
            Player.DisplayName = FString::Printf(TEXT("%s %s"), FirstNames[Random.Range(0, 11)], LastNames[Random.Range(0, 11)]);
            Player.Age = Random.Range(19, 34);
            Player.HeightCm = Random.Range(183, 216);
            Player.bLeftHanded = Random.ChancePerTenThousand(1100);
            Player.Position = PositionForIndex(PlayerIndex);
            Player.Archetype = static_cast<EPlayerArchetype>(Random.Range(0, 7));

            int32* Ratings[] = { &Player.Ratings.InsideScoring, &Player.Ratings.OutsideShooting,
                &Player.Ratings.Playmaking, &Player.Ratings.PerimeterDefense, &Player.Ratings.InteriorDefense,
                &Player.Ratings.Rebounding, &Player.Ratings.Athleticism, &Player.Ratings.Stamina };
            for (int32* Rating : Ratings) { *Rating = TeamQuality + Random.Range(-12, 12); }
            Player.Ratings.Potential = Random.Range(FMath::Max(45, TeamQuality), 92);
            Player.Ratings.WorkEthic = Random.Range(40, 90);
            Player.Ratings.Clamp();
            Player.Contract.SalaryMinorUnits = static_cast<int64>(Player.Ratings.Overall()) * 2200000LL;
            Player.Contract.YearsRemaining = Random.Range(1, 3);
            Team.Players.Add(Player);

            FPlayerState State;
            State.PlayerId = Player.PlayerId;
            State.Morale = Random.Range(45, 65);
            Team.PlayerStates.Add(State);
            Team.Rotation.OrderedPlayers.Add(Player.PlayerId);
        }

        const int32 MinuteTargets[15] = { 36, 35, 34, 33, 32, 20, 18, 14, 10, 8, 0, 0, 0, 0, 0 };
        for (int32 Index = 0; Index < 15; ++Index)
        {
            Team.Rotation.TargetMinutes.Add(Team.Players[Index].PlayerId, MinuteTargets[Index]);
        }
        Team.Tactics.PrimaryOption = Team.Players[0].PlayerId;
        Team.Tactics.SecondaryOption = Team.Players[1].PlayerId;
        League.Teams.Add(MoveTemp(Team));
    }

    League.Schedule = GenerateDoubleRoundRobin(League.Teams, Seed ^ 0x5343484544554C45ULL);
    return League;
}

TArray<FScheduledGame> FLeagueGenerator::GenerateDoubleRoundRobin(const TArray<FTeamState>& Teams, uint64 Seed)
{
    TArray<FScheduledGame> Result;
    if (Teams.Num() < 2 || Teams.Num() % 2 != 0) { return Result; }

    FDeterministicRandom Random(Seed);
    TArray<int32> Order;
    for (int32 Index = 0; Index < Teams.Num(); ++Index) { Order.Add(Index); }

    const int32 RoundsPerHalf = Teams.Num() - 1;
    const int32 GamesPerRound = Teams.Num() / 2;
    Result.Reserve(Teams.Num() * RoundsPerHalf);

    for (int32 Round = 0; Round < RoundsPerHalf; ++Round)
    {
        for (int32 Game = 0; Game < GamesPerRound; ++Game)
        {
            int32 Left = Order[Game];
            int32 Right = Order[Teams.Num() - 1 - Game];
            if ((Round + Game) % 2 != 0) { Swap(Left, Right); }

            FScheduledGame First;
            First.GameId = DeterministicGuid(Random);
            First.HomeTeamId = Teams[Left].TeamId;
            First.AwayTeamId = Teams[Right].TeamId;
            First.Round = Round;
            Result.Add(First);

            FScheduledGame Return = First;
            Return.GameId = DeterministicGuid(Random);
            Swap(Return.HomeTeamId, Return.AwayTeamId);
            Return.Round = Round + RoundsPerHalf;
            Result.Add(Return);
        }

        const int32 Last = Order.Pop(false);
        Order.Insert(Last, 1);
    }
    Result.Sort([](const FScheduledGame& A, const FScheduledGame& B) { return A.Round < B.Round; });
    return Result;
}

bool FLeagueGenerator::ValidateLeague(const FLeagueState& League, FString& OutError)
{
    if (League.Teams.Num() != 12 || League.Schedule.Num() != 132)
    {
        OutError = TEXT("The MVP league requires 12 teams and 132 total regular-season games.");
        return false;
    }
    TSet<FGuid> Teams;
    TMap<FGuid, int32> GamesPerTeam;
    TSet<FString> Pairings;
    for (const FTeamState& Team : League.Teams)
    {
        if (!Team.TeamId.IsValid() || Teams.Contains(Team.TeamId) || Team.Players.Num() != 15)
        {
            OutError = TEXT("Team identity or roster size is invalid.");
            return false;
        }
        Teams.Add(Team.TeamId);
        FString RotationError;
        if (!Team.Rotation.IsValid(RotationError)) { OutError = RotationError; return false; }
    }
    for (const FScheduledGame& Game : League.Schedule)
    {
        if (!Teams.Contains(Game.HomeTeamId) || !Teams.Contains(Game.AwayTeamId) || Game.HomeTeamId == Game.AwayTeamId)
        {
            OutError = TEXT("Schedule contains an invalid matchup.");
            return false;
        }
        GamesPerTeam.FindOrAdd(Game.HomeTeamId)++;
        GamesPerTeam.FindOrAdd(Game.AwayTeamId)++;
        Pairings.Add(Game.HomeTeamId.ToString() + TEXT("|") + Game.AwayTeamId.ToString());
    }
    for (const FGuid& TeamId : Teams)
    {
        if (GamesPerTeam.FindRef(TeamId) != 22)
        {
            OutError = TEXT("Every team must play exactly 22 regular-season games.");
            return false;
        }
    }
    if (Pairings.Num() != 132)
    {
        OutError = TEXT("Every ordered home/away pairing must occur exactly once.");
        return false;
    }
    return true;
}
