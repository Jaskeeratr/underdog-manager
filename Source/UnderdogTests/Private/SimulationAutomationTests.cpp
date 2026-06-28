#include "Misc/AutomationTest.h"
#include "DeterministicRandom.h"
#include "LeagueGenerator.h"
#include "LeagueService.h"
#include "MatchSimulator.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeterministicRandomTest,
    "Underdog.Core.DeterministicRandom",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDeterministicRandomTest::RunTest(const FString& Parameters)
{
    FDeterministicRandom A(123456789ULL);
    FDeterministicRandom B(123456789ULL);
    FDeterministicRandom C(987654321ULL);
    bool bObservedDifference = false;
    for (int32 Index = 0; Index < 100; ++Index)
    {
        const uint64 ValueA = A.NextUInt64();
        TestEqual(TEXT("Equal seeds retain equal streams"), ValueA, B.NextUInt64());
        bObservedDifference |= ValueA != C.NextUInt64();
    }
    TestTrue(TEXT("Different seeds diverge"), bObservedDifference);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLeagueGenerationTest,
    "Underdog.Simulation.LeagueGeneration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLeagueGenerationTest::RunTest(const FString& Parameters)
{
    const FLeagueState League = FLeagueGenerator::Generate(20260627ULL);
    FString Error;
    TestTrue(*FString::Printf(TEXT("League validates: %s"), *Error), FLeagueGenerator::ValidateLeague(League, Error));
    TestEqual(TEXT("Team count"), League.Teams.Num(), 12);
    TestEqual(TEXT("Schedule count"), League.Schedule.Num(), 132);
    for (const FTeamState& Team : League.Teams)
    {
        TestEqual(TEXT("Roster count"), Team.Players.Num(), 15);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatchDeterminismTest,
    "Underdog.Simulation.MatchDeterminismAndIntegrity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMatchDeterminismTest::RunTest(const FString& Parameters)
{
    const FLeagueState League = FLeagueGenerator::Generate(20260627ULL);
    const FScheduledGame& Game = League.Schedule[0];
    const FTeamState* Home = League.Teams.FindByPredicate([&Game](const FTeamState& Team) { return Team.TeamId == Game.HomeTeamId; });
    const FTeamState* Away = League.Teams.FindByPredicate([&Game](const FTeamState& Team) { return Team.TeamId == Game.AwayTeamId; });
    TestNotNull(TEXT("Home team resolves"), Home);
    TestNotNull(TEXT("Away team resolves"), Away);
    if (!Home || !Away) { return false; }

    FMatchSnapshot Snapshot;
    Snapshot.GameId = Game.GameId;
    Snapshot.Seed = 42;
    Snapshot.HomeTeam = *Home;
    Snapshot.AwayTeam = *Away;
    FPossessionMatchSimulator Simulator;
    const FMatchResult First = Simulator.Simulate(Snapshot);
    const FMatchResult Second = Simulator.Simulate(Snapshot);
    TestEqual(TEXT("Home score deterministic"), First.HomeScore, Second.HomeScore);
    TestEqual(TEXT("Away score deterministic"), First.AwayScore, Second.AwayScore);
    TestEqual(TEXT("Event count deterministic"), First.Events.Num(), Second.Events.Num());
    FString Error;
    TestTrue(*FString::Printf(TEXT("Result validates: %s"), *Error), First.Validate(Error));
    TestTrue(TEXT("Basketball match cannot end tied"), First.HomeScore != First.AwayScore);
    int32 RecordedHomePoints = 0;
    for (const FPlayerBoxScore& Box : First.HomeBoxScore) { RecordedHomePoints += Box.Points; }
    TestEqual(TEXT("Home player points reconcile"), RecordedHomePoints, First.HomeScore);
    TestEqual(TEXT("Final event retains home score"), First.Events.Last().HomeScore, First.HomeScore);
    TestEqual(TEXT("Final event retains away score"), First.Events.Last().AwayScore, First.AwayScore);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoundAdvanceAndStandingsTest,
    "Underdog.Simulation.RoundAdvanceAndStandings",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRoundAdvanceAndStandingsTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(20260627ULL);
    TArray<FMatchResult> Results;
    FString Error;
    TestTrue(*FString::Printf(TEXT("Round advances: %s"), *Error),
        FLeagueService::AdvanceCurrentRound(League, Results, Error));
    TestEqual(TEXT("One round resolves six games"), Results.Num(), 6);
    TestEqual(TEXT("Current round increments"), League.CurrentRound, 1);

    int32 Completed = 0;
    int32 TotalWins = 0;
    int32 TotalLosses = 0;
    for (const FScheduledGame& Game : League.Schedule) { Completed += Game.bComplete ? 1 : 0; }
    for (const FTeamState& Team : League.Teams) { TotalWins += Team.Wins; TotalLosses += Team.Losses; }
    TestEqual(TEXT("Six games marked complete"), Completed, 6);
    for (const FScheduledGame& Game : League.Schedule)
    {
        if (Game.bComplete)
        {
            TestTrue(TEXT("Completed game stores its score"), Game.HomeScore >= 0 && Game.AwayScore >= 0);
            TestTrue(TEXT("Completed game is not tied"), Game.HomeScore != Game.AwayScore);
        }
    }
    TestEqual(TEXT("Six wins recorded"), TotalWins, 6);
    TestEqual(TEXT("Six losses recorded"), TotalLosses, 6);
    FString LeagueError;
    TestTrue(TEXT("Mutated league remains valid"), FLeagueGenerator::ValidateLeague(League, LeagueError));

    const TArray<FTeamState> Standings = FLeagueService::GetStandings(League);
    TestEqual(TEXT("Standings contain all teams"), Standings.Num(), 12);
    for (int32 Index = 1; Index < Standings.Num(); ++Index)
    {
        TestTrue(TEXT("Standings are ordered by wins"), Standings[Index - 1].Wins >= Standings[Index].Wins);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFullRegularSeasonTest,
    "Underdog.Simulation.FullRegularSeason",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFullRegularSeasonTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(77ULL);
    FString Error;
    for (int32 Round = 0; Round < 22; ++Round)
    {
        TArray<FMatchResult> Results;
        if (!FLeagueService::AdvanceCurrentRound(League, Results, Error))
        {
            AddError(FString::Printf(TEXT("Round %d failed: %s"), Round, *Error));
            return false;
        }
    }
    TestEqual(TEXT("Regular season reaches round 22"), League.CurrentRound, 22);
    for (const FTeamState& Team : League.Teams)
    {
        TestEqual(TEXT("Every team completes 22 games"), Team.Wins + Team.Losses, 22);
    }
    TArray<FMatchResult> ExtraResults;
    TestFalse(TEXT("A 23rd regular-season round is rejected"),
        FLeagueService::AdvanceCurrentRound(League, ExtraResults, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeasonSoakTest,
    "Underdog.Simulation.SeasonSoak",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::StressFilter)

bool FSeasonSoakTest::RunTest(const FString& Parameters)
{
    FPossessionMatchSimulator Simulator;
    for (uint64 Season = 1; Season <= 100; ++Season)
    {
        const FLeagueState League = FLeagueGenerator::Generate(Season);
        FString LeagueError;
        if (!FLeagueGenerator::ValidateLeague(League, LeagueError))
        {
            AddError(FString::Printf(TEXT("Season %llu invalid: %s"), Season, *LeagueError));
            return false;
        }
        for (const FScheduledGame& Game : League.Schedule)
        {
            const FTeamState* Home = League.Teams.FindByPredicate([&Game](const FTeamState& Team) { return Team.TeamId == Game.HomeTeamId; });
            const FTeamState* Away = League.Teams.FindByPredicate([&Game](const FTeamState& Team) { return Team.TeamId == Game.AwayTeamId; });
            FMatchSnapshot Snapshot;
            Snapshot.GameId = Game.GameId;
            Snapshot.Seed = static_cast<int64>(Season ^ static_cast<uint64>(GetTypeHash(Game.GameId)));
            Snapshot.HomeTeam = *Home;
            Snapshot.AwayTeam = *Away;
            const FMatchResult Result = Simulator.Simulate(Snapshot);
            FString MatchError;
            if (!Result.Validate(MatchError))
            {
                AddError(FString::Printf(TEXT("Season %llu match invalid: %s"), Season, *MatchError));
                return false;
            }
        }
    }
    return true;
}

#endif
