#include "Misc/AutomationTest.h"
#include "DeterministicRandom.h"
#include "LeagueGenerator.h"
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
