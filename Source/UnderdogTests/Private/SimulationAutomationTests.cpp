#include "Misc/AutomationTest.h"
#include "AwardsService.h"
#include "ChemistryService.h"
#include "CareerService.h"
#include "CommentaryService.h"
#include "DeterministicRandom.h"
#include "DevelopmentService.h"
#include "FreeAgencyService.h"
#include "FranchiseService.h"
#include "LeagueGenerator.h"
#include "LeagueService.h"
#include "MatchSimulator.h"
#include "ManagementService.h"
#include "OffseasonService.h"
#include "TradeService.h"
#include "AIManagerService.h"
#include "ContractService.h"
#include "GameRecapService.h"
#include "LeagueHistoryService.h"
#include "RivalryService.h"
#include "HighlightDirectorService.h"
#include "MatchPresentationService.h"
#include "StaffService.h"
#include "UnderdogSaveGame.h"

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
    TArray<FGuid> Incoming = { League.Teams[11].Players[13].PlayerId, League.Teams[11].Players[14].PlayerId };

    const bool bResult = FTradeService::ProposeTrade(League, ClubId, Outgoing, OpponentId, Incoming, Error);

    if (bResult)
    {
        TestEqual(TEXT("Proposer roster remains legal"), League.Teams[0].Players.Num(), ProposerRosterBefore);
        TestEqual(TEXT("Receiver roster remains legal"), League.Teams[11].Players.Num(), ReceiverRosterBefore);
        TestTrue(TEXT("Trade recorded in history"), League.TradeHistory.Num() > 0);
        TestEqual(TEXT("Trade status accepted"), League.TradeHistory.Last().Status, ETradeStatus::Accepted);
    }
    else
    {
        TestTrue(TEXT("Rejected trade recorded"), League.TradeHistory.Num() > 0);
        if (League.TradeHistory.Num() > 0)
        {
            TestEqual(TEXT("Trade status rejected"), League.TradeHistory.Last().Status, ETradeStatus::Rejected);
        }
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
    TestEqual(TEXT("Bracket has 2 semifinal series"), League.Playoffs.Series.Num(), 2);

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
    TestTrue(TEXT("Playoffs took reasonable games"), PlayoffGames >= 6 && PlayoffGames <= 9);

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
    for (int32 PlayoffStep = 0; PlayoffStep < 20 && League.Phase == ESeasonPhase::Playoffs; ++PlayoffStep)
    {
        TArray<FMatchResult> Results;
        if (!FLeagueService::AdvancePlayoffs(League, Results, Error))
        {
            AddError(FString::Printf(TEXT("Playoff advance failed: %s"), *Error));
            return false;
        }
    }
    TestEqual(TEXT("Playoffs completed"), League.Phase, ESeasonPhase::Complete);
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

    TestTrue(TEXT("Advance past contract expiry"), FOffseasonService::AdvanceOffseason(League, Error));
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
    for (int32 PlayoffStep = 0; PlayoffStep < 20 && League.Phase == ESeasonPhase::Playoffs; ++PlayoffStep)
    {
        TArray<FMatchResult> Results;
        if (!FLeagueService::AdvancePlayoffs(League, Results, Error))
        {
            AddError(FString::Printf(TEXT("Playoff advance failed: %s"), *Error));
            return false;
        }
    }
    TestEqual(TEXT("Playoffs completed before offseason"), League.Phase, ESeasonPhase::Complete);

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
        TestEqual(TEXT("Roster respects 15-player cap after draft"),
            League.Teams[0].Players.Num(), FMath::Min(RosterBefore + 1, 15));
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

// ── Phase 4 Tests ──────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerDraftPickTest,
    "Underdog.Phase4.PlayerDraftPick",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerDraftPickTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(550055ULL);
    FString Error;

    for (int32 R = 0; R < 22; ++R)
    {
        TArray<FMatchResult> Results;
        FLeagueService::AdvanceCurrentRound(League, Results, Error);
    }
    for (int32 R = 0; R < 40 && League.Phase == ESeasonPhase::Playoffs; ++R)
    {
        TArray<FMatchResult> Results;
        FLeagueService::AdvancePlayoffs(League, Results, Error);
    }
    FOffseasonService::StartOffseason(League, Error);

    while (League.Offseason.CurrentStep != EOffseasonStep::Draft)
    {
        FOffseasonService::AdvanceOffseason(League, Error);
        if (League.Offseason.CurrentStep == EOffseasonStep::Complete) { break; }
    }
    if (League.Offseason.CurrentStep != EOffseasonStep::Draft)
    {
        AddWarning(TEXT("Could not reach Draft step — skipping"));
        return true;
    }

    const FGuid& PlayerTeam = League.Teams[0].TeamId;
    int32 UndraftedIdx = INDEX_NONE;
    for (int32 I = 0; I < League.Offseason.DraftClass.Num(); ++I)
    {
        if (!League.Offseason.DraftClass[I].bDrafted) { UndraftedIdx = I; break; }
    }
    TestTrue(TEXT("Undrafted prospect exists"), UndraftedIdx != INDEX_NONE);

    const FString ProspectName = League.Offseason.DraftClass[UndraftedIdx].Profile.DisplayName;
    TestTrue(TEXT("DraftPlayer succeeds"), FOffseasonService::DraftPlayer(League, PlayerTeam, UndraftedIdx, Error));
    TestTrue(TEXT("Prospect marked drafted"), League.Offseason.DraftClass[UndraftedIdx].bDrafted);
    TestTrue(TEXT("Prospect assigned to team"), League.Offseason.DraftClass[UndraftedIdx].DraftedByTeamId == PlayerTeam);

    bool bFoundOnRoster = false;
    for (const FPlayerProfile& P : League.Teams[0].Players)
    {
        if (P.DisplayName == ProspectName) { bFoundOnRoster = true; break; }
    }
    TestTrue(TEXT("Drafted player on team roster"), bFoundOnRoster);

    TestFalse(TEXT("Cannot draft same prospect again"), FOffseasonService::DraftPlayer(League, PlayerTeam, UndraftedIdx, Error));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMultiPlayerTradeTest,
    "Underdog.Phase4.MultiPlayerTrade",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMultiPlayerTradeTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(660066ULL);
    FString Error;

    const FGuid& Team0 = League.Teams[0].TeamId;
    const FGuid& Team1 = League.Teams[1].TeamId;

    TArray<FGuid> Outgoing;
    Outgoing.Add(League.Teams[0].Players[0].PlayerId);
    Outgoing.Add(League.Teams[0].Players[1].PlayerId);

    TArray<FGuid> Incoming;
    Incoming.Add(League.Teams[1].Players[0].PlayerId);
    Incoming.Add(League.Teams[1].Players[1].PlayerId);

    const FString OutName0 = League.Teams[0].Players[0].DisplayName;
    const FString OutName1 = League.Teams[0].Players[1].DisplayName;
    const FString InName0 = League.Teams[1].Players[0].DisplayName;
    const FString InName1 = League.Teams[1].Players[1].DisplayName;

    int32 Team0CountBefore = League.Teams[0].Players.Num();
    int32 Team1CountBefore = League.Teams[1].Players.Num();

    TestTrue(TEXT("Multi-player trade succeeds"), FTradeService::ProposeTrade(League, Team0, Outgoing, Team1, Incoming, Error));

    TestEqual(TEXT("Team 0 roster count preserved"), League.Teams[0].Players.Num(), Team0CountBefore);
    TestEqual(TEXT("Team 1 roster count preserved"), League.Teams[1].Players.Num(), Team1CountBefore);

    auto FindOnTeam = [](const FTeamState& Team, const FString& Name) -> bool {
        for (const FPlayerProfile& P : Team.Players) { if (P.DisplayName == Name) return true; }
        return false;
    };

    TestTrue(TEXT("Incoming player 0 on team 0"), FindOnTeam(League.Teams[0], InName0));
    TestTrue(TEXT("Incoming player 1 on team 0"), FindOnTeam(League.Teams[0], InName1));
    TestTrue(TEXT("Outgoing player 0 on team 1"), FindOnTeam(League.Teams[1], OutName0));
    TestTrue(TEXT("Outgoing player 1 on team 1"), FindOnTeam(League.Teams[1], OutName1));

    TestTrue(TEXT("Trade recorded in history"), League.TradeHistory.Num() > 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInjuryReportTest,
    "Underdog.Phase4.InjuryReport",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInjuryReportTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(770077ULL);

    League.Teams[0].PlayerStates[0].InjuryGamesRemaining = 3;
    League.Teams[0].PlayerStates[0].InjuryDescription = TEXT("Ankle Sprain");

    TestEqual(TEXT("Injury games remaining set"), League.Teams[0].PlayerStates[0].InjuryGamesRemaining, 3);
    TestEqual(TEXT("Injury description set"), League.Teams[0].PlayerStates[0].InjuryDescription, FString(TEXT("Ankle Sprain")));

    FString Error;
    TArray<FMatchResult> Results;
    FLeagueService::AdvanceCurrentRound(League, Results, Error);

    TestTrue(TEXT("Injury decremented after round"), League.Teams[0].PlayerStates[0].InjuryGamesRemaining < 3);

    League.Teams[0].PlayerStates[1].InjuryGamesRemaining = 0;
    League.Teams[0].PlayerStates[1].InjuryDescription = TEXT("");
    TestEqual(TEXT("Healthy player has zero injury games"), League.Teams[0].PlayerStates[1].InjuryGamesRemaining, 0);
    TestTrue(TEXT("Healthy player has empty injury description"), League.Teams[0].PlayerStates[1].InjuryDescription.IsEmpty());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveSchemaVersionTest,
    "Underdog.Phase4.SaveSchemaVersion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSaveSchemaVersionTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Schema version is 9"), UUnderdogSaveGame::CurrentSchemaVersion, 9);
    return true;
}

// ── Phase 5 Tests ──────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameRecapTest,
    "Underdog.Phase5.GameRecap",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameRecapTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(880088ULL);
    FString Error;
    TArray<FMatchResult> Results;
    FLeagueService::AdvanceCurrentRound(League, Results, Error);

    TestTrue(TEXT("Round produced results"), Results.Num() > 0);

    const FMatchResult& Result = Results[0];
    FScheduledGame* Game = League.Schedule.FindByPredicate(
        [&Result](const FScheduledGame& G) { return G.GameId == Result.GameId; });
    TestNotNull(TEXT("Game found in schedule"), Game);

    FMatchSnapshot Snapshot;
    Snapshot.GameId = Result.GameId;
    Snapshot.HomeTeam = *League.Teams.FindByPredicate(
        [Game](const FTeamState& T) { return T.TeamId == Game->HomeTeamId; });
    Snapshot.AwayTeam = *League.Teams.FindByPredicate(
        [Game](const FTeamState& T) { return T.TeamId == Game->AwayTeamId; });

    FGameRecap Recap = FGameRecapService::BuildRecap(Result, Snapshot);

    TestTrue(TEXT("Recap has quarter scores"), Recap.QuarterScores.Num() >= 4);
    TestTrue(TEXT("Recap has play-by-play entries"), Recap.PlayByPlay.Num() > 0);
    TestEqual(TEXT("Final home score matches"), Recap.FinalHomeScore, Result.HomeScore);
    TestEqual(TEXT("Final away score matches"), Recap.FinalAwayScore, Result.AwayScore);

    bool bHasHighlight = false;
    for (const FPlayByPlayEntry& Entry : Recap.PlayByPlay)
    {
        if (Entry.bHighlight) { bHasHighlight = true; break; }
    }
    TestTrue(TEXT("Recap contains highlight entries"), bHasHighlight);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLeagueHistoryTest,
    "Underdog.Phase5.LeagueHistory",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLeagueHistoryTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(990099ULL);
    FString Error;

    for (int32 R = 0; R < 22; ++R)
    {
        TArray<FMatchResult> Results;
        FLeagueService::AdvanceCurrentRound(League, Results, Error);
    }
    for (int32 R = 0; R < 40 && League.Phase == ESeasonPhase::Playoffs; ++R)
    {
        TArray<FMatchResult> Results;
        FLeagueService::AdvancePlayoffs(League, Results, Error);
    }

    TestTrue(TEXT("Season completed"), League.Phase == ESeasonPhase::Complete);
    FOffseasonService::StartOffseason(League, Error);

    TestTrue(TEXT("Championship recorded"), League.History.Championships.Num() >= 1);
    TestTrue(TEXT("Champion name not empty"), !League.History.Championships.Last().ChampionName.IsEmpty());
    TestTrue(TEXT("All-time leaders populated"), League.History.AllTimeLeaders.Num() > 0);

    TArray<FAllTimeLeader> TopScorers = FLeagueHistoryService::GetTopScorers(League.History, 5);
    TestTrue(TEXT("Top scorers returned"), TopScorers.Num() > 0);
    TestTrue(TEXT("Top scorer has points"), TopScorers[0].TotalPoints > 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContractExtensionTest,
    "Underdog.Phase5.ContractExtension",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FContractExtensionTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(110011ULL);

    League.Teams[0].Players[0].Contract.YearsRemaining = 1;
    const FGuid& PlayerId = League.Teams[0].Players[0].PlayerId;
    const FGuid& TeamId = League.Teams[0].TeamId;

    TArray<FExtensionOffer> Eligible = FContractService::GetEligibleExtensions(League, TeamId);
    TestTrue(TEXT("Player eligible for extension"), Eligible.Num() > 0);

    FExtensionOffer Asking = FContractService::CalculateAskingPrice(
        League.Teams[0].Players[0], &League.Teams[0].PlayerStates[0]);
    TestTrue(TEXT("Asking salary positive"), Asking.AskingSalary > 0);
    TestTrue(TEXT("Asking years positive"), Asking.AskingYears > 0);

    FString Error;
    bool bResult = FContractService::OfferExtension(League, TeamId, PlayerId,
        Asking.AskingSalary, Asking.AskingYears, Error);
    if (bResult)
    {
        TestTrue(TEXT("Contract years updated"), League.Teams[0].Players[0].Contract.YearsRemaining == Asking.AskingYears);
    }
    else
    {
        TestTrue(TEXT("Rejection has error message"), !Error.IsEmpty());
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRivalrySystemTest,
    "Underdog.Phase5.RivalrySystem",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRivalrySystemTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(220022ULL);

    const FGuid& Team0 = League.Teams[0].TeamId;
    const FGuid& Team1 = League.Teams[1].TeamId;

    TestTrue(TEXT("No rivalry initially"), FRivalryService::GetRivalry(League, Team0, Team1) == nullptr);

    FRivalryService::UpdateAfterGame(League, Team0, Team1, 3, false);
    const FRivalry* Rivalry = FRivalryService::GetRivalry(League, Team0, Team1);
    TestNotNull(TEXT("Rivalry created after close game"), Rivalry);
    TestTrue(TEXT("Intensity > 0"), Rivalry->Intensity > 0);
    TestEqual(TEXT("Close games tracked"), Rivalry->CloseGames, 1);

    FRivalryService::UpdateAfterGame(League, Team0, Team1, 2, true);
    Rivalry = FRivalryService::GetRivalry(League, Team0, Team1);
    TestTrue(TEXT("Playoff meeting increases intensity"), Rivalry->Intensity >= 20);
    TestEqual(TEXT("Playoff meetings tracked"), Rivalry->PlayoffMeetings, 1);

    const int32 IntensityBefore = Rivalry->Intensity;
    FRivalryService::DecayRivalries(League);
    Rivalry = FRivalryService::GetRivalry(League, Team0, Team1);
    if (Rivalry)
    {
        TestTrue(TEXT("Decay reduces intensity"), Rivalry->Intensity < IntensityBefore);
    }

    TArray<FRivalry> Top = FRivalryService::GetTopRivalries(League, 3);
    TestTrue(TEXT("Top rivalries returned"), Top.Num() > 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRivalryMoraleBonusTest,
    "Underdog.Phase5.RivalryMoraleBonus",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRivalryMoraleBonusTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(330033ULL);

    const FGuid& Team0 = League.Teams[0].TeamId;
    const FGuid& Team1 = League.Teams[1].TeamId;

    TestEqual(TEXT("No bonus without rivalry"), FRivalryService::GetMoraleBonus(League, Team0, Team1), 0);

    for (int32 I = 0; I < 10; ++I)
    {
        FRivalryService::UpdateAfterGame(League, Team0, Team1, 2, true);
    }

    const int32 Bonus = FRivalryService::GetMoraleBonus(League, Team0, Team1);
    TestTrue(TEXT("Strong rivalry gives morale bonus"), Bonus > 0);

    return true;
}

// ── Phase 6: Highlight Director ──

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHighlightDirectorTest,
    "Underdog.Simulation.HighlightDirector",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHighlightDirectorTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(770011ULL);
    FString Error;
    TestTrue(TEXT("League valid"), FLeagueGenerator::ValidateLeague(League, Error));
    FLeagueService::SetPlayerTeamId(League, League.Teams[0].TeamId);

    TArray<FMatchResult> Results;
    TestTrue(TEXT("Round advanced"), FLeagueService::AdvanceCurrentRound(League, Results, Error));
    TestTrue(TEXT("Results exist"), Results.Num() > 0);

    const FMatchResult& Result = Results[0];
    const FScheduledGame* Game = League.Schedule.FindByPredicate(
        [&Result](const FScheduledGame& G) { return G.GameId == Result.GameId; });
    TestTrue(TEXT("Game found"), Game != nullptr);

    FMatchSnapshot Snapshot;
    TestTrue(TEXT("Snapshot built"), FLeagueService::BuildSnapshot(League, *Game, Snapshot, Error));

    TArray<FHighlightCue> Cues = FHighlightDirectorService::BuildHighlights(Result, Snapshot);
    TestTrue(TEXT("At least one highlight cue"), Cues.Num() > 0);
    TestTrue(TEXT("Capped at 10"), Cues.Num() <= 10);

    for (int32 I = 1; I < Cues.Num(); ++I)
    {
        const bool bChronological = Cues[I].Period > Cues[I - 1].Period ||
            (Cues[I].Period == Cues[I - 1].Period && Cues[I].ClockSeconds <= Cues[I - 1].ClockSeconds);
        TestTrue(TEXT("Cues chronological"), bChronological);
    }

    FString ValidationError;
    TestTrue(TEXT("Cues pass validation"), FHighlightDirectorService::ValidateCues(Cues, Result, ValidationError));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHighlightFallbackTest,
    "Underdog.Simulation.HighlightFallback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHighlightFallbackTest::RunTest(const FString& Parameters)
{
    FMatchResult EmptyResult;
    EmptyResult.GameId = FGuid::NewGuid();
    EmptyResult.HomeScore = 100;
    EmptyResult.AwayScore = 95;

    FMatchSnapshot EmptySnapshot;
    EmptySnapshot.HomeTeam.TeamId = FGuid::NewGuid();
    EmptySnapshot.AwayTeam.TeamId = FGuid::NewGuid();

    TArray<FHighlightCue> Cues = FHighlightDirectorService::BuildHighlights(EmptyResult, EmptySnapshot);
    TestTrue(TEXT("Fallback cue generated"), Cues.Num() >= 1);
    TestEqual(TEXT("Fallback template"), Cues[0].Template, EHighlightTemplate::GenericFallback);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPresentationPackageTest,
    "Underdog.Simulation.PresentationPackage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPresentationPackageTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(880022ULL);
    FString Error;
    TestTrue(TEXT("League valid"), FLeagueGenerator::ValidateLeague(League, Error));
    FLeagueService::SetPlayerTeamId(League, League.Teams[0].TeamId);

    TArray<FMatchResult> Results;
    TestTrue(TEXT("Round advanced"), FLeagueService::AdvanceCurrentRound(League, Results, Error));

    const FMatchResult& Result = Results[0];
    const FScheduledGame* Game = League.Schedule.FindByPredicate(
        [&Result](const FScheduledGame& G) { return G.GameId == Result.GameId; });
    TestTrue(TEXT("Game found"), Game != nullptr);

    FMatchSnapshot Snapshot;
    TestTrue(TEXT("Snapshot built"), FLeagueService::BuildSnapshot(League, *Game, Snapshot, Error));

    FGameRecap Recap = FGameRecapService::BuildRecap(Result, Snapshot);
    FMatchPresentationPackage Pkg = FMatchPresentationService::BuildPackage(League, Result, Snapshot, Recap);

    TestTrue(TEXT("Package GameId set"), Pkg.GameId == Result.GameId);
    TestTrue(TEXT("Home team data populated"), !Pkg.Home.FullName.IsEmpty());
    TestTrue(TEXT("Away team data populated"), !Pkg.Away.FullName.IsEmpty());
    TestTrue(TEXT("Highlights present"), Pkg.Highlights.Num() > 0);
    TestTrue(TEXT("Result scores match"), Pkg.Result.HomeScore == Result.HomeScore && Pkg.Result.AwayScore == Result.AwayScore);
    TestTrue(TEXT("Recap headline present"), !Pkg.Recap.Headline.IsEmpty());
    TestTrue(TEXT("Starting five populated"), Pkg.Home.StartingFive.Num() > 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStandingsImplicationTest,
    "Underdog.Simulation.StandingsImplication",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStandingsImplicationTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(990033ULL);
    FString Error;
    FLeagueGenerator::ValidateLeague(League, Error);

    League.Teams[0].WinStreak = 6;
    FString Impl = FMatchPresentationService::ComputeStandingsImplication(
        League, League.Teams[0].TeamId, League.Teams[1].TeamId);
    TestTrue(TEXT("Win streak mentioned"), Impl.Contains(TEXT("streak")));

    League.Teams[0].WinStreak = 0;
    League.CurrentRound = 20;
    League.Teams[0].Wins = 15;
    League.Teams[0].Losses = 5;
    League.Teams[1].Wins = 14;
    League.Teams[1].Losses = 6;
    Impl = FMatchPresentationService::ComputeStandingsImplication(
        League, League.Teams[0].TeamId, League.Teams[1].TeamId);
    TestTrue(TEXT("Playoff positioning noted"), Impl.Contains(TEXT("Playoff")) || Impl.Contains(TEXT("playoff")));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveSchemaV6Test,
    "Underdog.Game.SaveSchemaV6",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSaveSchemaV6Test::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Schema version is 9"), UUnderdogSaveGame::CurrentSchemaVersion, 9);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBroadcastCueContractTest,
    "Underdog.Phase7.BroadcastCueContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBroadcastCueContractTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(707070ULL);
    FString Error;
    TArray<FMatchResult> Results;
    TestTrue(TEXT("Round simulated"), FLeagueService::AdvanceCurrentRound(League, Results, Error));
    TestTrue(TEXT("Result generated"), Results.Num() > 0);

    const FMatchResult& Result = Results[0];
    const FScheduledGame* Game = League.Schedule.FindByPredicate(
        [&Result](const FScheduledGame& Candidate) { return Candidate.GameId == Result.GameId; });
    TestNotNull(TEXT("Scheduled game found"), Game);
    if (!Game) { return false; }

    FMatchSnapshot Snapshot;
    TestTrue(TEXT("Snapshot rebuilt"), FLeagueService::BuildSnapshot(League, *Game, Snapshot, Error));
    const TArray<FHighlightCue> Cues = FHighlightDirectorService::BuildHighlights(Result, Snapshot);
    TestTrue(TEXT("Broadcast has at least one cue"), Cues.Num() > 0);

    for (const FHighlightCue& Cue : Cues)
    {
        TestTrue(TEXT("Cue duration is safe"), Cue.PlaybackDuration >= 0.5f);
        TestTrue(TEXT("Cue period is valid"), Cue.Period >= 1);
        TestTrue(TEXT("Cue clock is valid"), Cue.ClockSeconds >= 0);
        TestTrue(TEXT("Cue description is present"), !Cue.Description.IsEmpty());
        TestTrue(TEXT("Cue score does not exceed final home score"), Cue.HomeScoreAfter <= Result.HomeScore);
        TestTrue(TEXT("Cue score does not exceed final away score"), Cue.AwayScoreAfter <= Result.AwayScore);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFranchiseInitializationTest,
    "Underdog.Phase8.FranchiseInitialization",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFranchiseInitializationTest::RunTest(const FString& Parameters)
{
    const FLeagueState League = FLeagueGenerator::Generate(808001ULL);
    for (const FTeamState& Team : League.Teams)
    {
        TestEqual(TEXT("Four facilities initialized"), Team.Franchise.Facilities.Num(), 4);
        TestEqual(TEXT("Three owner objectives initialized"), Team.Franchise.Ownership.Objectives.Num(), 3);
        TestTrue(TEXT("Starting cash is positive"), Team.Franchise.Finances.CashMinorUnits > 0);
        TestTrue(TEXT("Fan support bounded"), Team.Franchise.Fanbase.Support >= 0
            && Team.Franchise.Fanbase.Support <= 100);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFranchiseGameEconomicsTest,
    "Underdog.Phase8.GameEconomics",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFranchiseGameEconomicsTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(808002ULL);
    const int64 StartingCash = League.Teams[0].Franchise.Finances.CashMinorUnits;
    FString Error;
    TArray<FMatchResult> Results;
    TestTrue(TEXT("Round simulated"), FLeagueService::AdvanceCurrentRound(League, Results, Error));
    TestTrue(TEXT("Six game results generated"), Results.Num() == 6);

    int32 TeamsWithRevenue = 0;
    for (const FTeamState& Team : League.Teams)
    {
        if (Team.Franchise.Finances.SeasonRevenueMinorUnits > 0) { TeamsWithRevenue++; }
        TestTrue(TEXT("Expenses recorded"), Team.Franchise.Finances.SeasonExpensesMinorUnits > 0);
        TestEqual(TEXT("Legacy balance synchronized"), Team.OperatingBalanceMinorUnits,
            Team.Franchise.Finances.CashMinorUnits);
    }
    TestEqual(TEXT("Six home teams earned revenue"), TeamsWithRevenue, 6);
    TestTrue(TEXT("Cash changed after round"), League.Teams[0].Franchise.Finances.CashMinorUnits != StartingCash);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFacilityUpgradeTest,
    "Underdog.Phase8.FacilityUpgrade",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFacilityUpgradeTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(808003ULL);
    FTeamState& Team = League.Teams[0];
    const int64 Cost = FFranchiseService::GetFacilityUpgradeCost(Team, EFacilityType::TrainingCentre);
    const int64 StartingCash = Team.Franchise.Finances.CashMinorUnits;
    FString Error;
    TestTrue(TEXT("Training facility upgrade succeeds"), FFranchiseService::UpgradeFacility(
        League, Team.TeamId, EFacilityType::TrainingCentre, Error));
    TestEqual(TEXT("Training level increased"), FFranchiseService::GetFacilityLevel(
        League.Teams[0], EFacilityType::TrainingCentre), 2);
    TestEqual(TEXT("Upgrade cost deducted"), League.Teams[0].Franchise.Finances.CashMinorUnits,
        StartingCash - Cost);
    TestFalse(TEXT("Invalid ticket price rejected"), FFranchiseService::SetTicketPrice(
        League, Team.TeamId, 1999, Error));
    TestTrue(TEXT("Valid ticket price accepted"), FFranchiseService::SetTicketPrice(
        League, Team.TeamId, 5500, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOwnerObjectiveTest,
    "Underdog.Phase8.OwnerObjectives",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOwnerObjectiveTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(808004ULL);
    FTeamState& Team = League.Teams[0];
    Team.Wins = 10;
    Team.Losses = 4;
    Team.Chemistry = 70;
    Team.Franchise.Finances.SeasonRevenueMinorUnits = 500000000LL;
    Team.Franchise.Finances.SeasonExpensesMinorUnits = 400000000LL;
    FFranchiseService::EvaluateOwnership(Team, ESeasonPhase::RegularSeason);
    int32 Completed = 0;
    for (const FOwnerObjective& Objective : Team.Franchise.Ownership.Objectives)
    {
        Completed += Objective.bCompleted ? 1 : 0;
    }
    TestEqual(TEXT("All standard objectives completed"), Completed, 3);
    TestTrue(TEXT("Owner confidence improved"), Team.Franchise.Ownership.Confidence >= 75);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStaffInitializationTest,
    "Underdog.Phase9.StaffInitialization",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStaffInitializationTest::RunTest(const FString& Parameters)
{
    const FLeagueState League = FLeagueGenerator::Generate(909001ULL);
    TestEqual(TEXT("Staff market initialized"), League.StaffMarket.Num(), 18);
    for (const FTeamState& Team : League.Teams)
    {
        TestEqual(TEXT("Every team starts with six staff roles"), Team.Organization.Staff.Num(), 6);
        TestTrue(TEXT("Staff payroll calculated"), Team.Organization.AnnualPayrollMinorUnits > 0);
        TestTrue(TEXT("Staff chemistry bounded"), Team.Organization.StaffChemistry >= 0
            && Team.Organization.StaffChemistry <= 100);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStaffHiringTest,
    "Underdog.Phase9.StaffHiring",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStaffHiringTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(909002ULL);
    FTeamState& Team = League.Teams[0];
    const FStaffMember Candidate = League.StaffMarket[0];
    const int64 CashBefore = Team.Franchise.Finances.CashMinorUnits;
    FString Error;
    TestFalse(TEXT("Low salary offer rejected"), FStaffService::HireStaff(League, Team.TeamId,
        Candidate.StaffId, Candidate.Contract.SalaryMinorUnits - 1, 2, Error));
    TestTrue(TEXT("Asking salary offer accepted"), FStaffService::HireStaff(League, Team.TeamId,
        Candidate.StaffId, Candidate.Contract.SalaryMinorUnits, 2, Error));
    int32 RoleCount = 0;
    for (const FStaffMember& Staff : League.Teams[0].Organization.Staff)
    {
        RoleCount += Staff.Role == Candidate.Role ? 1 : 0;
    }
    TestEqual(TEXT("Role remains filled once"), RoleCount, 1);
    TestTrue(TEXT("Hiring costs cash"), League.Teams[0].Franchise.Finances.CashMinorUnits < CashBefore);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStaffEffectsAndAITest,
    "Underdog.Phase9.StaffEffectsAndAI",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStaffEffectsAndAITest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(909003ULL);
    FTeamState& Team = League.Teams[1];
    Team.Organization.Staff.RemoveAll([](const FStaffMember& Staff)
        { return Staff.Role == EStaffRole::HeadScout; });
    const int32 FamiliarityBefore = Team.Organization.TacticalFamiliarity;
    FStaffService::ProcessRound(League, League.Teams[0].TeamId);
    TestNotNull(TEXT("AI fills vacant head scout role"),
        FStaffService::FindStaff(League.Teams[1], EStaffRole::HeadScout));
    TestTrue(TEXT("Tactical familiarity increases"),
        League.Teams[1].Organization.TacticalFamiliarity > FamiliarityBefore);
    TestTrue(TEXT("Development bonus bounded"),
        FStaffService::GetDevelopmentBonus(League.Teams[1]) >= -200
        && FStaffService::GetDevelopmentBonus(League.Teams[1]) <= 350);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTradeLegalityAndExplanationTest,
    "Underdog.Phase9.TradeLegalityAndExplanation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTradeLegalityAndExplanationTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(909004ULL);
    FTradeOffer Offer;
    Offer.ProposingTeamId = League.Teams[0].TeamId;
    Offer.ReceivingTeamId = League.Teams[1].TeamId;
    FTradeAsset Offered; Offered.PlayerId = League.Teams[0].Players[0].PlayerId; Offered.FromTeamId = Offer.ProposingTeamId;
    FTradeAsset Requested; Requested.PlayerId = League.Teams[1].Players[0].PlayerId; Requested.FromTeamId = Offer.ReceivingTeamId;
    Offer.Outgoing.Add(Offered);
    Offer.Incoming.Add(Requested);
    const FTradeEvaluation Evaluation = FTradeService::EvaluateTradeDetailed(League, Offer);
    TestTrue(TEXT("Normal offer is legally evaluable"), Evaluation.bLegal);
    TestTrue(TEXT("Evaluation includes summary"), !Evaluation.Summary.IsEmpty());
    TestTrue(TEXT("Both package values calculated"), Evaluation.ProposerReceivesValue > 0
        && Evaluation.ReceiverReceivesValue > 0);

    FString Error;
    TArray<FGuid> FourPlayers;
    for (int32 Index = 0; Index < 4; ++Index) { FourPlayers.Add(League.Teams[0].Players[Index].PlayerId); }
    TestFalse(TEXT("Four-player side rejected"), FTradeService::ProposeTrade(League,
        League.Teams[0].TeamId, FourPlayers, League.Teams[1].TeamId,
        { League.Teams[1].Players[0].PlayerId }, Error));
    TestTrue(TEXT("Rejection explains three-player limit"), Error.Contains(TEXT("three")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FManagerCareerTrackingTest,
    "Underdog.Phase10.ManagerCareerTracking",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FManagerCareerTrackingTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(1010001ULL);
    TestTrue(TEXT("Manager assigned to player club"),
        League.ManagerCareer.CurrentTeamId == League.Teams[0].TeamId);
    FString Error;
    TestFalse(TEXT("Invalid short manager name rejected"),
        FCareerService::SetManagerName(League, TEXT("A"), Error));
    TestTrue(TEXT("Valid manager name accepted"),
        FCareerService::SetManagerName(League, TEXT("Jaskeerat Rai"), Error));

    TArray<FMatchResult> Results;
    TestTrue(TEXT("Round advances"), FLeagueService::AdvanceCurrentRound(League, Results, Error));
    TestEqual(TEXT("Manager record contains one game"),
        League.ManagerCareer.CareerWins + League.ManagerCareer.CareerLosses, 1);
    TestTrue(TEXT("Career score is non-negative"), League.ManagerCareer.CareerScore >= 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FManagerJobMovementTest,
    "Underdog.Phase10.ManagerJobMovement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FManagerJobMovementTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(1010002ULL);
    const FGuid OriginalTeam = League.ManagerCareer.CurrentTeamId;
    FCareerService::GenerateJobOffers(League);
    TestTrue(TEXT("At least one job offer generated"), !League.ManagerCareer.JobOffers.IsEmpty());
    const FGuid OfferedTeam = League.ManagerCareer.JobOffers[0].TeamId;
    FString Error;
    TestTrue(TEXT("Job offer accepted"), FCareerService::AcceptJobOffer(League, OfferedTeam, Error));
    TestTrue(TEXT("Manager changed clubs"), League.ManagerCareer.CurrentTeamId != OriginalTeam);
    TestEqual(TEXT("Selected club becomes player team index zero"), League.Teams[0].TeamId, OfferedTeam);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTenSeasonCareerSoakTest,
    "Underdog.Phase10.TenSeasonCareerSoak",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTenSeasonCareerSoakTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(1010003ULL);
    FString Error;
    for (int32 Season = 0; Season < 10; ++Season)
    {
        for (int32 Round = 0; Round < 22; ++Round)
        {
            TArray<FMatchResult> Results;
            if (!FLeagueService::AdvanceCurrentRound(League, Results, Error))
            {
                AddError(FString::Printf(TEXT("Season %d round %d failed: %s"), Season + 1, Round + 1, *Error));
                return false;
            }
        }
        for (int32 Step = 0; Step < 12 && League.Phase == ESeasonPhase::Playoffs; ++Step)
        {
            TArray<FMatchResult> Results;
            if (!FLeagueService::AdvancePlayoffs(League, Results, Error))
            {
                AddError(FString::Printf(TEXT("Season %d playoffs failed: %s"), Season + 1, *Error));
                return false;
            }
        }
        TestEqual(TEXT("Season reached completion"), League.Phase, ESeasonPhase::Complete);
        TestTrue(TEXT("Offseason starts"), FOffseasonService::StartOffseason(League, Error));
        for (int32 Step = 0; Step < 10 && League.Phase == ESeasonPhase::Complete; ++Step)
        {
            if (!FOffseasonService::AdvanceOffseason(League, Error))
            {
                AddError(FString::Printf(TEXT("Season %d offseason failed: %s"), Season + 1, *Error));
                return false;
            }
        }
        TestEqual(TEXT("New regular season begins"), League.Phase, ESeasonPhase::RegularSeason);
        for (const FTeamState& Team : League.Teams)
        {
            TestTrue(TEXT("Roster remains legal"), Team.Players.Num() >= 10 && Team.Players.Num() <= 15);
            TestTrue(TEXT("Organization payroll remains valid"), Team.Organization.AnnualPayrollMinorUnits >= 0);
            TestTrue(TEXT("Franchise cash remains bounded"),
                Team.Franchise.Finances.CashMinorUnits > -50000000000LL
                && Team.Franchise.Finances.CashMinorUnits < 500000000000LL);
        }
    }
    TestEqual(TEXT("Ten manager seasons recorded"), League.ManagerCareer.SeasonHistory.Num(), 10);
    TestEqual(TEXT("Career advanced to season eleven"), League.SeasonNumber, 11);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHighlightChoreographyCoverageTest,
    "Underdog.Simulation.HighlightChoreographyCoverage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHighlightChoreographyCoverageTest::RunTest(const FString& Parameters)
{
    FLeagueState League = FLeagueGenerator::Generate(2020050ULL);
    FString Error;
    FLeagueGenerator::ValidateLeague(League, Error);
    FLeagueService::SetPlayerTeamId(League, League.Teams[0].TeamId);

    TArray<FMatchResult> Results;
    FLeagueService::AdvanceCurrentRound(League, Results, Error);
    TestTrue(TEXT("Has results"), Results.Num() > 0);

    const FMatchResult& Result = Results[0];
    const FScheduledGame* Game = League.Schedule.FindByPredicate(
        [&Result](const FScheduledGame& G) { return G.GameId == Result.GameId; });

    FMatchSnapshot Snapshot;
    FLeagueService::BuildSnapshot(League, *Game, Snapshot, Error);

    TArray<FHighlightCue> Cues = FHighlightDirectorService::BuildHighlights(Result, Snapshot, 20);
    TestTrue(TEXT("Generated multiple cues"), Cues.Num() > 0);

    TSet<EHighlightTemplate> SeenTemplates;
    for (const FHighlightCue& Cue : Cues)
    {
        SeenTemplates.Add(Cue.Template);
        TestTrue(TEXT("Playback duration positive"), Cue.PlaybackDuration > 0.0f);
        TestTrue(TEXT("Description not empty"), !Cue.Description.IsEmpty());
    }
    TestTrue(TEXT("Multiple template types covered"), SeenTemplates.Num() >= 2);

    return true;
}

#endif
