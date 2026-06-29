#include "Misc/AutomationTest.h"
#include "DeterministicRandom.h"
#include "LeagueGenerator.h"
#include "LeagueService.h"
#include "MatchSimulator.h"
#include "ManagementService.h"
#include "TradeService.h"
#include "AIManagerService.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FManagementSystemsTest,
    "Underdog.Management.TrainingScoutingAndRotation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FManagementSystemsTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(20260627ULL);
    const FGuid ClubId = League.Teams[0].TeamId;
    FString Error;
    TestTrue(TEXT("Automatic rotation succeeds"), FManagementService::AutoBuildRotation(League, ClubId, Error));
    TestTrue(TEXT("Automatic rotation validates"), League.Teams[0].Rotation.IsValid(Error));

    TestTrue(TEXT("Training plan can be changed"), FManagementService::SetTrainingPlan(League, ClubId,
        ETrainingFocus::Shooting, ETrainingIntensity::High, Error));
    TestEqual(TEXT("Training focus persists"), League.Teams[0].TrainingPlan.Focus, ETrainingFocus::Shooting);
    const int32 FatigueBefore = League.Teams[0].PlayerStates[0].Fatigue;

    for (int32 Index = 0; Index < 3; ++Index)
    {
        TestTrue(TEXT("Scouting slot accepts assignment"), FManagementService::AssignScout(
            League, ClubId, League.Teams[1].Players[Index].PlayerId, Error));
    }
    TestFalse(TEXT("Fourth concurrent scout is rejected"), FManagementService::AssignScout(
        League, ClubId, League.Teams[1].Players[3].PlayerId, Error));
    League.CurrentRound = 3;
    FManagementService::ProcessRound(League);
    TestTrue(TEXT("High intensity adds training load"),
        League.Teams[0].PlayerStates[0].Fatigue > FatigueBefore);
    TestEqual(TEXT("All three reports complete"), League.ScoutingReports.Num(), 3);
    for (const FScoutingReport& Report : League.ScoutingReports)
    {
        TestTrue(TEXT("Overall range is ordered"), Report.OverallMin <= Report.OverallMax);
        TestTrue(TEXT("Potential range is ordered"), Report.PotentialMin <= Report.PotentialMax);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTradeSystemTest,
    "Underdog.Management.TradeSystem",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTradeSystemTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(20260627ULL);
    const FGuid ClubId = League.Teams[0].TeamId;
    const FGuid OpponentId = League.Teams[11].TeamId;

    const FPlayerProfile& OutPlayer = League.Teams[0].Players[14];
    const FPlayerProfile& InPlayer = League.Teams[11].Players[0];
    const int32 OutValue = FTradeService::CalculatePlayerValue(OutPlayer,
        League.Teams[0].PlayerStates[14]);
    const int32 InValue = FTradeService::CalculatePlayerValue(InPlayer,
        League.Teams[11].PlayerStates[0]);
    TestTrue(TEXT("Player value is positive"), OutValue > 0 && InValue > 0);

    const int32 ProposerRosterBefore = League.Teams[0].Players.Num();
    const int32 ReceiverRosterBefore = League.Teams[11].Players.Num();

    FString Error;
    TArray<FGuid> Outgoing = { League.Teams[0].Players[13].PlayerId, League.Teams[0].Players[14].PlayerId };
    TArray<FGuid> Incoming = { League.Teams[11].Players[14].PlayerId };

    const bool bResult = FTradeService::ProposeTrade(League, ClubId, Outgoing, OpponentId, Incoming, Error);

    if (bResult)
    {
        TestEqual(TEXT("Proposer roster adjusted"), League.Teams[0].Players.Num(), ProposerRosterBefore - 1);
        TestEqual(TEXT("Receiver roster adjusted"), League.Teams[11].Players.Num(), ReceiverRosterBefore + 1);
        TestTrue(TEXT("Trade recorded in history"), League.TradeHistory.Num() > 0);
        TestEqual(TEXT("Trade status accepted"), League.TradeHistory.Last().Status, ETradeStatus::Accepted);
    }
    else
    {
        TestTrue(TEXT("Rejected trade recorded"), League.TradeHistory.Num() > 0);
        TestEqual(TEXT("Trade status rejected"), League.TradeHistory.Last().Status, ETradeStatus::Rejected);
    }

    FString SelfError;
    TestFalse(TEXT("Self-trade rejected"), FTradeService::ProposeTrade(League, ClubId,
        { League.Teams[0].Players[0].PlayerId }, ClubId,
        { League.Teams[0].Players[1].PlayerId }, SelfError));

    League.CurrentRound = 18;
    FString DeadlineError;
    TestFalse(TEXT("Post-deadline trade rejected"), FTradeService::ProposeTrade(League, ClubId,
        { League.Teams[0].Players[0].PlayerId }, OpponentId,
        { League.Teams[11].Players[0].PlayerId }, DeadlineError));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayoffBracketTest,
    "Underdog.Simulation.PlayoffBracket",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayoffBracketTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(42ULL);
    FLeagueService::SetPlayerTeamId(League, League.Teams[0].TeamId);
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
    TestEqual(TEXT("Phase transitions to playoffs"), League.Phase, ESeasonPhase::Playoffs);
    TestEqual(TEXT("Bracket has 4 first-round series"), League.Playoffs.Series.Num(), 4);

    for (const FPlayoffSeries& Series : League.Playoffs.Series)
    {
        TestTrue(TEXT("Higher seed team valid"), Series.HigherSeedTeamId.IsValid());
        TestTrue(TEXT("Lower seed team valid"), Series.LowerSeedTeamId.IsValid());
        TestTrue(TEXT("Different teams"), Series.HigherSeedTeamId != Series.LowerSeedTeamId);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFullSeasonToChampionTest,
    "Underdog.Simulation.FullSeasonToChampion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFullSeasonToChampionTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(99ULL);
    FLeagueService::SetPlayerTeamId(League, League.Teams[0].TeamId);
    FString Error;

    for (int32 Round = 0; Round < 22; ++Round)
    {
        TArray<FMatchResult> Results;
        if (!FLeagueService::AdvanceCurrentRound(League, Results, Error))
        {
            AddError(FString::Printf(TEXT("Regular season round %d failed: %s"), Round, *Error));
            return false;
        }
    }
    TestEqual(TEXT("Enters playoffs"), League.Phase, ESeasonPhase::Playoffs);

    int32 PlayoffGames = 0;
    while (League.Phase == ESeasonPhase::Playoffs && PlayoffGames < 100)
    {
        TArray<FMatchResult> Results;
        if (!FLeagueService::AdvancePlayoffs(League, Results, Error))
        {
            AddError(FString::Printf(TEXT("Playoff game %d failed: %s"), PlayoffGames, *Error));
            return false;
        }
        PlayoffGames += Results.Num();
    }

    TestEqual(TEXT("Season completes"), League.Phase, ESeasonPhase::Complete);
    TestTrue(TEXT("Champion declared"), League.Playoffs.ChampionTeamId.IsValid());
    TestTrue(TEXT("Playoffs took reasonable games"), PlayoffGames >= 12 && PlayoffGames <= 28);

    const FTeamState* Champion = League.Teams.FindByPredicate(
        [&League](const FTeamState& T) { return T.TeamId == League.Playoffs.ChampionTeamId; });
    TestNotNull(TEXT("Champion team exists"), Champion);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIManagerTest,
    "Underdog.Management.AIManager",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIManagerTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(20260627ULL);
    const FGuid PlayerTeamId = League.Teams[0].TeamId;

    FAIManagerService::ProcessRound(League, PlayerTeamId);

    bool bAnyScoutAssigned = false;
    for (const FScoutingAssignment& Assignment : League.ScoutingAssignments)
    {
        if (Assignment.RequestedByTeamId != PlayerTeamId)
        {
            bAnyScoutAssigned = true;
            break;
        }
    }
    TestTrue(TEXT("AI teams assigned scouts"), bAnyScoutAssigned);

    bool bAnyTrainingChanged = false;
    for (int32 Index = 1; Index < League.Teams.Num(); ++Index)
    {
        if (League.Teams[Index].TrainingPlan.Focus != ETrainingFocus::Balanced
            || League.Teams[Index].TrainingPlan.Intensity != ETrainingIntensity::Normal)
        {
            bAnyTrainingChanged = true;
            break;
        }
    }
    TestTrue(TEXT("AI teams adjusted training"), bAnyTrainingChanged);

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
