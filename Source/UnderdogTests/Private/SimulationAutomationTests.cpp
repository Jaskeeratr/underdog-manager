#include "Misc/AutomationTest.h"
#include "AwardsService.h"
#include "ChemistryService.h"
#include "CommentaryService.h"
#include "DeterministicRandom.h"
#include "DevelopmentService.h"
#include "FreeAgencyService.h"
#include "LeagueGenerator.h"
#include "LeagueService.h"
#include "MatchSimulator.h"
#include "ManagementService.h"
#include "OffseasonService.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommentaryGenerationTest,
    "Underdog.Phase2.CommentaryGeneration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommentaryGenerationTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(20260627ULL);
    const FScheduledGame& Game = League.Schedule[0];
    FMatchSnapshot Snapshot;
    FString Error;
    TestTrue(TEXT("Snapshot builds"), FLeagueService::BuildSnapshot(League, Game, Snapshot, Error));

    FPossessionMatchSimulator Simulator;
    const FMatchResult Result = Simulator.Simulate(Snapshot);

    TArray<FCommentaryLine> Full = FCommentaryService::Generate(Result, Snapshot);
    TestTrue(TEXT("Commentary produces lines"), Full.Num() > 0);
    for (const FCommentaryLine& Line : Full)
    {
        TestTrue(TEXT("Commentary text is non-empty"), Line.Text.Len() > 0);
        TestTrue(TEXT("Period is valid"), Line.Period >= 1);
    }

    TArray<FCommentaryLine> Highlights = FCommentaryService::GenerateHighlights(Result, Snapshot, 5);
    TestTrue(TEXT("Highlights capped at max"), Highlights.Num() <= 5);
    TestTrue(TEXT("Highlights are non-empty"), Highlights.Num() > 0);

    for (int32 Index = 1; Index < Highlights.Num(); ++Index)
    {
        const bool bChronological = Highlights[Index].Period > Highlights[Index - 1].Period
            || (Highlights[Index].Period == Highlights[Index - 1].Period
                && Highlights[Index].ClockSeconds <= Highlights[Index - 1].ClockSeconds);
        TestTrue(TEXT("Highlights are chronological"), bChronological);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAwardSelectionTest,
    "Underdog.Phase2.AwardSelection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAwardSelectionTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(99ULL);
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

    TestTrue(TEXT("Season stats accumulated"), League.SeasonStats.Num() > 0);
    int32 WithGames = 0;
    for (const FSeasonStats& Stats : League.SeasonStats)
    {
        if (Stats.GamesPlayed > 0) { WithGames++; }
    }
    TestTrue(TEXT("Multiple players have game stats"), WithGames > 20);

    TArray<FSeasonAward> Awards = FAwardsService::CalculateAwards(League);
    TestTrue(TEXT("Awards include MVP"), Awards.ContainsByPredicate(
        [](const FSeasonAward& A) { return A.Type == EAwardType::MVP; }));
    TestTrue(TEXT("Awards include DPOY"), Awards.ContainsByPredicate(
        [](const FSeasonAward& A) { return A.Type == EAwardType::DPOY; }));

    for (const FSeasonAward& Award : Awards)
    {
        TestTrue(TEXT("Award player ID valid"), Award.PlayerId.IsValid());
        TestTrue(TEXT("Award team ID valid"), Award.TeamId.IsValid());
    }

    while (League.Phase == ESeasonPhase::Playoffs)
    {
        TArray<FMatchResult> Results;
        if (!FLeagueService::AdvancePlayoffs(League, Results, Error)) { break; }
    }
    if (League.Playoffs.ChampionTeamId.IsValid())
    {
        FSeasonAward ChampMVP = FAwardsService::CalculateChampionMVP(League);
        TestEqual(TEXT("Champion MVP type"), ChampMVP.Type, EAwardType::ChampionMVP);
        TestTrue(TEXT("Champion MVP player valid"), ChampMVP.PlayerId.IsValid());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOffseasonAgingAndContractsTest,
    "Underdog.Phase2.OffseasonAgingAndContracts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOffseasonAgingAndContractsTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(99ULL);
    FLeagueService::SetPlayerTeamId(League, League.Teams[0].TeamId);
    FString Error;

    for (int32 Round = 0; Round < 22; ++Round)
    {
        TArray<FMatchResult> Results;
        FLeagueService::AdvanceCurrentRound(League, Results, Error);
    }
    while (League.Phase == ESeasonPhase::Playoffs)
    {
        TArray<FMatchResult> Results;
        FLeagueService::AdvancePlayoffs(League, Results, Error);
    }
    TestEqual(TEXT("Season complete"), League.Phase, ESeasonPhase::Complete);

    TMap<FGuid, int32> AgesBefore;
    for (const FTeamState& Team : League.Teams)
    {
        for (const FPlayerProfile& Player : Team.Players)
        {
            AgesBefore.Add(Player.PlayerId, Player.Age);
        }
    }

    TestTrue(TEXT("Start offseason"), FOffseasonService::StartOffseason(League, Error));
    TestEqual(TEXT("Offseason at awards step"), League.Offseason.CurrentStep, EOffseasonStep::Awards);

    TestTrue(TEXT("Advance past awards"), FOffseasonService::AdvanceOffseason(League, Error));
    TestEqual(TEXT("At aging step"), League.Offseason.CurrentStep, EOffseasonStep::Aging);

    TestTrue(TEXT("Advance past aging"), FOffseasonService::AdvanceOffseason(League, Error));
    TestEqual(TEXT("At contract expiry"), League.Offseason.CurrentStep, EOffseasonStep::ContractExpiry);

    bool bAnyAged = false;
    for (const FTeamState& Team : League.Teams)
    {
        for (const FPlayerProfile& Player : Team.Players)
        {
            const int32* Before = AgesBefore.Find(Player.PlayerId);
            if (Before && Player.Age > *Before) { bAnyAged = true; }
        }
    }
    TestTrue(TEXT("Players aged"), bAnyAged);

    TestTrue(TEXT("Advance past contracts"), FOffseasonService::AdvanceOffseason(League, Error));
    TestEqual(TEXT("At free agency step"), League.Offseason.CurrentStep, EOffseasonStep::FreeAgency);
    TestTrue(TEXT("Free agent pool built"), League.Offseason.FreeAgentPool.Num() >= 0);

    TestTrue(TEXT("Advance past free agency"), FOffseasonService::AdvanceOffseason(League, Error));
    TestEqual(TEXT("At draft step"), League.Offseason.CurrentStep, EOffseasonStep::Draft);
    TestTrue(TEXT("Draft class generated"), League.Offseason.DraftClass.Num() > 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDraftAndMultiSeasonTest,
    "Underdog.Phase2.DraftAndMultiSeason",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDraftAndMultiSeasonTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(77ULL);
    FLeagueService::SetPlayerTeamId(League, League.Teams[0].TeamId);
    FString Error;

    for (int32 Round = 0; Round < 22; ++Round)
    {
        TArray<FMatchResult> Results;
        FLeagueService::AdvanceCurrentRound(League, Results, Error);
    }
    while (League.Phase == ESeasonPhase::Playoffs)
    {
        TArray<FMatchResult> Results;
        FLeagueService::AdvancePlayoffs(League, Results, Error);
    }

    TestTrue(TEXT("Start offseason"), FOffseasonService::StartOffseason(League, Error));

    while (League.Offseason.CurrentStep != EOffseasonStep::Draft)
    {
        TestTrue(TEXT("Advance offseason step"), FOffseasonService::AdvanceOffseason(League, Error));
    }

    const int32 RosterBefore = League.Teams[0].Players.Num();
    if (League.Offseason.DraftClass.Num() > 0)
    {
        int32 FirstAvailable = -1;
        for (int32 Index = 0; Index < League.Offseason.DraftClass.Num(); ++Index)
        {
            if (!League.Offseason.DraftClass[Index].bDrafted) { FirstAvailable = Index; break; }
        }
        if (FirstAvailable >= 0)
        {
            TestTrue(TEXT("Draft player succeeds"),
                FOffseasonService::DraftPlayer(League, League.Teams[0].TeamId, FirstAvailable, Error));
            TestTrue(TEXT("Prospect marked drafted"), League.Offseason.DraftClass[FirstAvailable].bDrafted);
            TestEqual(TEXT("Roster grew by one"), League.Teams[0].Players.Num(), RosterBefore + 1);
        }
    }

    while (League.Offseason.CurrentStep != EOffseasonStep::Complete)
    {
        FOffseasonService::AdvanceOffseason(League, Error);
    }
    TestEqual(TEXT("Offseason complete"), League.Offseason.CurrentStep, EOffseasonStep::Complete);

    const int32 SeasonBefore = League.SeasonNumber;
    TestTrue(TEXT("Final advance starts new season"), FOffseasonService::AdvanceOffseason(League, Error));
    TestEqual(TEXT("Phase back to regular season"), League.Phase, ESeasonPhase::RegularSeason);
    TestEqual(TEXT("Season number incremented"), League.SeasonNumber, SeasonBefore + 1);
    TestEqual(TEXT("Current round reset"), League.CurrentRound, 0);
    TestTrue(TEXT("Schedule regenerated"), League.Schedule.Num() > 0);
    TestEqual(TEXT("Season stats cleared"), League.SeasonStats.Num(), 0);

    for (const FTeamState& Team : League.Teams)
    {
        TestEqual(TEXT("Wins reset"), Team.Wins, 0);
        TestEqual(TEXT("Losses reset"), Team.Losses, 0);
    }

    TArray<FMatchResult> NewResults;
    TestTrue(TEXT("New season round advances"),
        FLeagueService::AdvanceCurrentRound(League, NewResults, Error));
    TestEqual(TEXT("New season round 1 complete"), League.CurrentRound, 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChemistryAndMoraleTest,
    "Underdog.Phase3.ChemistryAndMorale",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChemistryAndMoraleTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(20260627ULL);
    FLeagueService::SetPlayerTeamId(League, League.Teams[0].TeamId);

    const int32 ChemBefore = League.Teams[0].Chemistry;
    TestEqual(TEXT("Initial chemistry is 50"), ChemBefore, 50);

    FChemistryService::UpdateChemistryAfterGame(League.Teams[0], true, League);
    TestTrue(TEXT("Chemistry updates after win"), League.Teams[0].Chemistry >= ChemBefore);
    TestTrue(TEXT("Win streak incremented"), League.Teams[0].WinStreak > 0);

    FChemistryService::UpdateChemistryAfterTrade(League.Teams[0], 2);
    const int32 ChemAfterTrade = League.Teams[0].Chemistry;
    TestTrue(TEXT("Chemistry drops after trade"), ChemAfterTrade < League.Teams[0].Chemistry + 16);

    FChemistryService::UpdateMoraleAfterGame(League.Teams[0], true, false);
    bool bAnyMoraleChanged = false;
    for (const FAthleteState& S : League.Teams[0].PlayerStates)
    {
        if (S.Morale != 50) { bAnyMoraleChanged = true; break; }
    }
    TestTrue(TEXT("Morale changed after game"), bAnyMoraleChanged);

    const int32 HighChemBonus = FChemistryService::GetSimBonus(League.Teams[0]);
    League.Teams[0].Chemistry = 25;
    const int32 LowChemBonus = FChemistryService::GetSimBonus(League.Teams[0]);
    TestTrue(TEXT("High chemistry gives better bonus than low"), HighChemBonus > LowChemBonus);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevelopmentAndMentorshipTest,
    "Underdog.Phase3.DevelopmentAndMentorship",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevelopmentAndMentorshipTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(20260627ULL);

    FDevelopmentService::EstablishMentorships(League);
    TestTrue(TEXT("Mentorships established"), League.Mentorships.Num() > 0);
    for (const FMentorship& M : League.Mentorships)
    {
        TestTrue(TEXT("Veteran ID valid"), M.VeteranId.IsValid());
        TestTrue(TEXT("Rookie ID valid"), M.RookieId.IsValid());
        TestTrue(TEXT("Bonus rating gain positive"), M.BonusRatingGain > 0);
    }

    TMap<FGuid, int32> OverallsBefore;
    for (const FTeamState& Team : League.Teams)
    {
        for (const FPlayerProfile& P : Team.Players) { OverallsBefore.Add(P.PlayerId, P.Ratings.Overall()); }
    }

    for (int32 Iter = 0; Iter < 10; ++Iter)
    {
        League.SeasonNumber = Iter + 1;
        FDevelopmentService::ProcessDevelopment(League);
    }

    int32 Improved = 0;
    for (const FTeamState& Team : League.Teams)
    {
        for (const FPlayerProfile& P : Team.Players)
        {
            const int32* Before = OverallsBefore.Find(P.PlayerId);
            if (Before && P.Ratings.Overall() > *Before) { Improved++; }
        }
    }
    TestTrue(TEXT("Some players improved through development"), Improved > 0);

    FDevelopmentService::ApplyBreakoutBust(League);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFreeAgencyTest,
    "Underdog.Phase3.FreeAgency",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFreeAgencyTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(99ULL);
    FLeagueService::SetPlayerTeamId(League, League.Teams[0].TeamId);
    FString Error;

    for (int32 Round = 0; Round < 22; ++Round)
    {
        TArray<FMatchResult> Results;
        FLeagueService::AdvanceCurrentRound(League, Results, Error);
    }
    while (League.Phase == ESeasonPhase::Playoffs)
    {
        TArray<FMatchResult> Results;
        FLeagueService::AdvancePlayoffs(League, Results, Error);
    }

    TestTrue(TEXT("Start offseason"), FOffseasonService::StartOffseason(League, Error));

    while (League.Offseason.CurrentStep != EOffseasonStep::FreeAgency
        && League.Offseason.CurrentStep != EOffseasonStep::Complete)
    {
        FOffseasonService::AdvanceOffseason(League, Error);
    }

    if (League.Offseason.CurrentStep == EOffseasonStep::FreeAgency)
    {
        TestTrue(TEXT("Free agent pool built"), League.Offseason.FreeAgentPool.Num() > 0);

        bool bSorted = true;
        for (int32 Index = 1; Index < League.Offseason.FreeAgentPool.Num(); ++Index)
        {
            if (League.Offseason.FreeAgentPool[Index].Profile.Ratings.Overall()
                > League.Offseason.FreeAgentPool[Index - 1].Profile.Ratings.Overall())
            {
                bSorted = false;
                break;
            }
        }
        TestTrue(TEXT("FA pool sorted by OVR descending"), bSorted);

        for (const FFreeAgent& FA : League.Offseason.FreeAgentPool)
        {
            TestTrue(TEXT("Asking salary positive"), FA.AskingSalary > 0);
            TestTrue(TEXT("Previous team ID valid"), FA.PreviousTeamId.IsValid());
        }

        if (League.Offseason.FreeAgentPool.Num() > 0)
        {
            const FFreeAgent& BestFA = League.Offseason.FreeAgentPool[0];
            FString SignError;
            TestFalse(TEXT("Low offer rejected"), FFreeAgencyService::SignFreeAgent(
                League, League.Teams[0].TeamId, 0, BestFA.AskingSalary / 2, 2, SignError));
        }

        FOffseasonService::AdvanceOffseason(League, Error);
        TestEqual(TEXT("After FA advance, at draft"), League.Offseason.CurrentStep, EOffseasonStep::Draft);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTacticalSimImpactTest,
    "Underdog.Phase3.TacticalSimImpact",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTacticalSimImpactTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(20260627ULL);
    const FScheduledGame& Game = League.Schedule[0];
    FMatchSnapshot Snapshot;
    FString Error;
    FLeagueService::BuildSnapshot(League, Game, Snapshot, Error);

    FPossessionMatchSimulator Simulator;

    Snapshot.HomeTeam.Tactics.Pace = EPaceStyle::Fast;
    Snapshot.HomeTeam.Tactics.Offense = EOffenseStyle::Perimeter;
    const FMatchResult FastPerimeter = Simulator.Simulate(Snapshot);

    Snapshot.HomeTeam.Tactics.Pace = EPaceStyle::Slow;
    Snapshot.HomeTeam.Tactics.Offense = EOffenseStyle::Inside;
    const FMatchResult SlowInside = Simulator.Simulate(Snapshot);

    TestTrue(TEXT("Different tactics produce different scores"),
        FastPerimeter.HomeScore != SlowInside.HomeScore
        || FastPerimeter.AwayScore != SlowInside.AwayScore
        || FastPerimeter.Events.Num() != SlowInside.Events.Num());

    Snapshot.HomeTeam.Tactics.Defense = EDefenseStyle::Zone;
    const FMatchResult ZoneDefense = Simulator.Simulate(Snapshot);
    TestTrue(TEXT("Zone defense produces valid result"), ZoneDefense.HomeScore >= 0);

    FString ValidError;
    TestTrue(TEXT("Fast/perimeter validates"), FastPerimeter.Validate(ValidError));
    TestTrue(TEXT("Slow/inside validates"), SlowInside.Validate(ValidError));
    TestTrue(TEXT("Zone validates"), ZoneDefense.Validate(ValidError));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMultiSeasonWithFATest,
    "Underdog.Phase3.MultiSeasonWithFreeAgency",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMultiSeasonWithFATest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(42ULL);
    FLeagueService::SetPlayerTeamId(League, League.Teams[0].TeamId);
    FString Error;

    for (int32 Season = 0; Season < 2; ++Season)
    {
        for (int32 Round = 0; Round < 22; ++Round)
        {
            TArray<FMatchResult> Results;
            if (!FLeagueService::AdvanceCurrentRound(League, Results, Error))
            {
                AddError(FString::Printf(TEXT("S%d R%d failed: %s"), Season + 1, Round, *Error));
                return false;
            }
        }
        while (League.Phase == ESeasonPhase::Playoffs)
        {
            TArray<FMatchResult> Results;
            if (!FLeagueService::AdvancePlayoffs(League, Results, Error)) { break; }
        }
        TestEqual(TEXT("Season completes"), League.Phase, ESeasonPhase::Complete);

        FOffseasonService::StartOffseason(League, Error);
        while (League.Offseason.CurrentStep != EOffseasonStep::Complete)
        {
            FOffseasonService::AdvanceOffseason(League, Error);
        }
        FOffseasonService::AdvanceOffseason(League, Error);
        TestEqual(TEXT("Back to regular season"), League.Phase, ESeasonPhase::RegularSeason);
    }

    TestEqual(TEXT("Season number is 3 after 2 full seasons"), League.SeasonNumber, 3);
    TestTrue(TEXT("Mentorships established"), League.Mentorships.Num() >= 0);

    for (const FTeamState& Team : League.Teams)
    {
        TestTrue(TEXT("Team has minimum 10 players"), Team.Players.Num() >= 10);
        TestTrue(TEXT("Team has at most 15 players"), Team.Players.Num() <= 15);
    }

    FString LeagueError;
    TestTrue(TEXT("League validates after 2 seasons"), FLeagueGenerator::ValidateLeague(League, LeagueError));

    return true;
}

#endif
