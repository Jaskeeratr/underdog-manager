#include "ManagementDashboardWidget.h"

#include "CommentaryService.h"
#include "LeagueGameSubsystem.h"
#include "LeagueService.h"
#include "UnderdogCoreTypes.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Styling/CoreStyle.h"

namespace DashboardStyle
{
    const FLinearColor Background(0.018f, 0.027f, 0.043f, 1.0f);
    const FLinearColor Sidebar(0.030f, 0.043f, 0.064f, 1.0f);
    const FLinearColor Card(0.045f, 0.062f, 0.086f, 1.0f);
    const FLinearColor CardRaised(0.060f, 0.080f, 0.108f, 1.0f);
    const FLinearColor Accent(0.96f, 0.55f, 0.12f, 1.0f);
    const FLinearColor AccentSoft(0.30f, 0.17f, 0.06f, 1.0f);
    const FLinearColor Primary(0.94f, 0.96f, 0.98f, 1.0f);
    const FLinearColor Secondary(0.55f, 0.63f, 0.72f, 1.0f);
    const FLinearColor Success(0.27f, 0.82f, 0.55f, 1.0f);

    FSlateChildSize Size(float Value, ESlateSizeRule::Type Rule = ESlateSizeRule::Fill)
    {
        FSlateChildSize Result(Rule);
        Result.Value = Value;
        return Result;
    }

    FString Position(EPlayerPosition Value)
    {
        switch (Value)
        {
        case EPlayerPosition::PG: return TEXT("PG");
        case EPlayerPosition::SG: return TEXT("SG");
        case EPlayerPosition::SF: return TEXT("SF");
        case EPlayerPosition::PF: return TEXT("PF");
        case EPlayerPosition::C: return TEXT("C");
        default: return TEXT("—");
        }
    }
}

void UManagementDashboardWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshDashboard();
}

TSharedRef<SWidget> UManagementDashboardWidget::RebuildWidget()
{
    if (!WidgetTree) { Initialize(); }
    if (WidgetTree && !WidgetTree->RootWidget) { BuildLayout(); }
    return Super::RebuildWidget();
}

UTextBlock* UManagementDashboardWidget::MakeText(
    const FString& Text, int32 Size, const FLinearColor& Color, bool bBold)
{
    UTextBlock* Widget = WidgetTree->ConstructWidget<UTextBlock>();
    Widget->SetText(FText::FromString(Text));
    Widget->SetColorAndOpacity(FSlateColor(Color));
    Widget->SetFont(FCoreStyle::GetDefaultFontStyle(bBold ? TEXT("Bold") : TEXT("Regular"), Size));
    return Widget;
}

UBorder* UManagementDashboardWidget::MakeCard(const FLinearColor& Color)
{
    UBorder* CardWidget = WidgetTree->ConstructWidget<UBorder>();
    CardWidget->SetBrushColor(Color);
    CardWidget->SetPadding(FMargin(20.0f));
    return CardWidget;
}

UButton* UManagementDashboardWidget::MakeNavigationButton(const FString& Label, bool bActive)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>();
    Button->SetBackgroundColor(bActive ? DashboardStyle::AccentSoft : DashboardStyle::Sidebar);
    UTextBlock* LabelText = MakeText(Label, 14,
        bActive ? DashboardStyle::Accent : DashboardStyle::Secondary, bActive);
    LabelText->SetJustification(ETextJustify::Left);
    Button->SetContent(LabelText);
    return Button;
}

void UManagementDashboardWidget::BuildLayout()
{
    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
    WidgetTree->RootWidget = Root;

    UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
    Background->SetBrushColor(DashboardStyle::Background);
    Root->AddChildToOverlay(Background);

    UHorizontalBox* Shell = WidgetTree->ConstructWidget<UHorizontalBox>();
    UOverlaySlot* ShellOverlaySlot = Root->AddChildToOverlay(Shell);
    ShellOverlaySlot->SetHorizontalAlignment(HAlign_Fill);
    ShellOverlaySlot->SetVerticalAlignment(VAlign_Fill);

    UBorder* Sidebar = WidgetTree->ConstructWidget<UBorder>();
    Sidebar->SetBrushColor(DashboardStyle::Sidebar);
    Sidebar->SetPadding(FMargin(20.0f, 28.0f));
    UHorizontalBoxSlot* SidebarSlot = Shell->AddChildToHorizontalBox(Sidebar);
    SidebarSlot->SetSize(DashboardStyle::Size(0.19f));

    UVerticalBox* NavigationBox = WidgetTree->ConstructWidget<UVerticalBox>();
    Sidebar->SetContent(NavigationBox);
    NavigationBox->AddChildToVerticalBox(MakeText(TEXT("UNDERDOG"), 25, DashboardStyle::Primary, true));
    NavigationBox->AddChildToVerticalBox(MakeText(TEXT("BASKETBALL OPERATIONS"), 9, DashboardStyle::Accent, true))
        ->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 34.0f));

    const FString Labels[] = { TEXT("OVERVIEW"), TEXT("ROSTER"), TEXT("SCHEDULE"),
        TEXT("STANDINGS"), TEXT("SCOUTING"), TEXT("TRAINING"), TEXT("TRADES"), TEXT("PLAYOFFS"),
        TEXT("AWARDS"), TEXT("OFFSEASON") };
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Labels); ++Index)
    {
        UButton* NavigationButton = MakeNavigationButton(Labels[Index], Index == 0);
        switch (Index)
        {
        case 0: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowOverview); break;
        case 1: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowRoster); break;
        case 2: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowSchedule); break;
        case 3: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowStandings); break;
        case 4: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowScouting); break;
        case 5: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowTraining); break;
        case 6: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowTrades); break;
        case 7: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowPlayoffs); break;
        case 8: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowAwards); break;
        case 9: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowOffseason); break;
        default: break;
        }
        NavigationBox->AddChildToVerticalBox(NavigationButton)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
    }

    ScreenSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>();
    UHorizontalBoxSlot* SwitcherSlot = Shell->AddChildToHorizontalBox(ScreenSwitcher);
    SwitcherSlot->SetSize(DashboardStyle::Size(0.81f));
    SwitcherSlot->SetPadding(FMargin(34.0f, 26.0f, 34.0f, 24.0f));

    UVerticalBox* Main = WidgetTree->ConstructWidget<UVerticalBox>();
    ScreenSwitcher->AddChild(Main);

    UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
    Main->AddChildToVerticalBox(Header)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
    UVerticalBox* HeaderCopy = WidgetTree->ConstructWidget<UVerticalBox>();
    Header->AddChildToHorizontalBox(HeaderCopy)->SetSize(DashboardStyle::Size(1.0f));
    ClubNameText = MakeText(TEXT("CALGARY CHINOOKS"), 30, DashboardStyle::Primary, true);
    HeaderCopy->AddChildToVerticalBox(ClubNameText);
    SeasonText = MakeText(TEXT("REGULAR SEASON"), 11, DashboardStyle::Secondary, true);
    HeaderCopy->AddChildToVerticalBox(SeasonText)->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));

    SimulateButton = WidgetTree->ConstructWidget<UButton>();
    SimulateButton->SetBackgroundColor(DashboardStyle::Accent);
    SimulateButtonText = MakeText(TEXT("SIMULATE ROUND  >"), 14, DashboardStyle::Background, true);
    SimulateButton->SetContent(SimulateButtonText);
    SimulateButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleSimulateRound);
    UHorizontalBoxSlot* ActionSlot = Header->AddChildToHorizontalBox(SimulateButton);
    ActionSlot->SetSize(DashboardStyle::Size(1.0f, ESlateSizeRule::Automatic));
    ActionSlot->SetVerticalAlignment(VAlign_Center);
    ActionSlot->SetPadding(FMargin(24.0f, 0.0f));

    UHorizontalBox* Metrics = WidgetTree->ConstructWidget<UHorizontalBox>();
    Main->AddChildToVerticalBox(Metrics)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
    auto AddMetric = [this, Metrics](const FString& Label, TObjectPtr<UTextBlock>& Value, const FLinearColor& ValueColor)
    {
        UBorder* CardWidget = MakeCard(DashboardStyle::Card);
        UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
        CardWidget->SetContent(Content);
        Content->AddChildToVerticalBox(MakeText(Label, 9, DashboardStyle::Secondary, true));
        Value = MakeText(TEXT("—"), 24, ValueColor, true);
        Content->AddChildToVerticalBox(Value)->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));
        UHorizontalBoxSlot* Slot = Metrics->AddChildToHorizontalBox(CardWidget);
        Slot->SetSize(DashboardStyle::Size(1.0f));
        Slot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));
    };
    AddMetric(TEXT("SEASON RECORD"), RecordText, DashboardStyle::Primary);
    AddMetric(TEXT("CLUB BALANCE"), BalanceText, DashboardStyle::Success);

    UBorder* MatchCard = MakeCard(DashboardStyle::CardRaised);
    UVerticalBox* MatchContent = WidgetTree->ConstructWidget<UVerticalBox>();
    MatchCard->SetContent(MatchContent);
    MatchContent->AddChildToVerticalBox(MakeText(TEXT("NEXT MATCHUP"), 9, DashboardStyle::Accent, true));
    NextOpponentText = MakeText(TEXT("Schedule pending"), 17, DashboardStyle::Primary, true);
    MatchContent->AddChildToVerticalBox(NextOpponentText)->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));
    MatchupDetailsText = MakeText(TEXT(""), 10, DashboardStyle::Secondary);
    MatchContent->AddChildToVerticalBox(MatchupDetailsText)->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
    Metrics->AddChildToHorizontalBox(MatchCard)->SetSize(DashboardStyle::Size(1.35f));

    UHorizontalBox* Tables = WidgetTree->ConstructWidget<UHorizontalBox>();
    UVerticalBoxSlot* TablesSlot = Main->AddChildToVerticalBox(Tables);
    TablesSlot->SetSize(DashboardStyle::Size(1.0f));

    UBorder* RosterCard = MakeCard(DashboardStyle::Card);
    UVerticalBox* RosterContent = WidgetTree->ConstructWidget<UVerticalBox>();
    RosterCard->SetContent(RosterContent);
    RosterContent->AddChildToVerticalBox(MakeText(TEXT("ACTIVE ROTATION"), 12, DashboardStyle::Primary, true))
        ->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
    RosterList = WidgetTree->ConstructWidget<UVerticalBox>();
    RosterContent->AddChildToVerticalBox(RosterList);
    UHorizontalBoxSlot* RosterSlot = Tables->AddChildToHorizontalBox(RosterCard);
    RosterSlot->SetSize(DashboardStyle::Size(1.65f));
    RosterSlot->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));

    UBorder* StandingsCard = MakeCard(DashboardStyle::Card);
    UVerticalBox* StandingsContent = WidgetTree->ConstructWidget<UVerticalBox>();
    StandingsCard->SetContent(StandingsContent);
    StandingsContent->AddChildToVerticalBox(MakeText(TEXT("LEAGUE TABLE"), 12, DashboardStyle::Primary, true))
        ->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
    StandingsList = WidgetTree->ConstructWidget<UVerticalBox>();
    StandingsContent->AddChildToVerticalBox(StandingsList);
    Tables->AddChildToHorizontalBox(StandingsCard)->SetSize(DashboardStyle::Size(1.0f));

    StatusText = MakeText(TEXT("Front office systems online"), 10, DashboardStyle::Secondary);
    Main->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(2.0f, 12.0f, 0.0f, 0.0f));

    ScreenSwitcher->AddChild(MakeDetailScreen(TEXT("TEAM MANAGEMENT"), TEXT("Roster & Rotation"),
        TEXT("Review your squad, roles, fitness, morale, and target minutes."), RosterDetailList));
    ScreenSwitcher->AddChild(MakeDetailScreen(TEXT("SEASON OPERATIONS"), TEXT("Schedule"),
        TEXT("Track completed results and every upcoming regular-season fixture."), ScheduleList));
    ScreenSwitcher->AddChild(MakeDetailScreen(TEXT("LEAGUE"), TEXT("Standings"),
        TEXT("Head-to-head, point differential, and scoring determine tied positions."), StandingsDetailList));
    ScreenSwitcher->AddChild(MakeDetailScreen(TEXT("RECRUITMENT"), TEXT("Scouting Centre"),
        TEXT("Assign up to three scouts. Reports complete after two to three rounds."), ScoutingList));
    ScreenSwitcher->AddChild(MakeDetailScreen(TEXT("PLAYER DEVELOPMENT"), TEXT("Training"),
        TEXT("Choose a weekly focus and balance development against fatigue and injury risk."), TrainingList));
    ScreenSwitcher->AddChild(MakeDetailScreen(TEXT("TRANSACTIONS"), TEXT("Trade Centre"),
        TEXT("Propose trades with other teams. The trade deadline is round 16."), TradeList));
    ScreenSwitcher->AddChild(MakeDetailScreen(TEXT("POSTSEASON"), TEXT("Playoff Bracket"),
        TEXT("Top eight teams compete in best-of-seven series to determine the champion."), PlayoffList));
    ScreenSwitcher->AddChild(MakeDetailScreen(TEXT("RESULTS"), TEXT("Round Results"),
        TEXT("Box scores and outcomes from the most recent round of games."), ResultsList));
    ScreenSwitcher->AddChild(MakeDetailScreen(TEXT("RECOGNITION"), TEXT("Season Awards"),
        TEXT("MVP, Defensive Player of the Year, Rookie of the Year, and Most Improved."), AwardsList));
    ScreenSwitcher->AddChild(MakeDetailScreen(TEXT("FRONT OFFICE"), TEXT("Offseason"),
        TEXT("Step through aging, contracts, the draft, and re-signing to begin a new season."), OffseasonList));
}

UVerticalBox* UManagementDashboardWidget::MakeDetailScreen(const FString& Eyebrow, const FString& Title,
    const FString& Description, TObjectPtr<UVerticalBox>& OutList)
{
    UVerticalBox* Screen = WidgetTree->ConstructWidget<UVerticalBox>();
    Screen->AddChildToVerticalBox(MakeText(Eyebrow, 10, DashboardStyle::Accent, true));
    Screen->AddChildToVerticalBox(MakeText(Title, 29, DashboardStyle::Primary, true))
        ->SetPadding(FMargin(0.0f, 5.0f, 0.0f, 0.0f));
    UTextBlock* DescriptionText = MakeText(Description, 11, DashboardStyle::Secondary);
    DescriptionText->SetAutoWrapText(true);
    Screen->AddChildToVerticalBox(DescriptionText)->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 22.0f));
    UBorder* ContentCard = MakeCard(DashboardStyle::Card);
    OutList = WidgetTree->ConstructWidget<UVerticalBox>();
    ContentCard->SetContent(OutList);
    UVerticalBoxSlot* CardSlot = Screen->AddChildToVerticalBox(ContentCard);
    CardSlot->SetSize(DashboardStyle::Size(1.0f));
    return Screen;
}

void UManagementDashboardWidget::RefreshDashboard()
{
    ULeagueGameSubsystem* LeagueSubsystem = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>();
    if (!LeagueSubsystem) { return; }
    if (!LeagueSubsystem->HasLeague())
    {
        FString Error;
        if (!LeagueSubsystem->StartNewLeague(20260627, Error))
        {
            StatusText->SetText(FText::FromString(Error));
            return;
        }
    }

    const FLeagueState League = LeagueSubsystem->GetLeague();
    if (League.Teams.Num() == 0) { return; }
    const FTeamState& Club = League.Teams[0];
    ClubNameText->SetText(FText::FromString(FString::Printf(
        TEXT("%s %s"), *Club.City.ToUpper(), *Club.Nickname.ToUpper())));
    if (League.Phase == ESeasonPhase::Playoffs)
    {
        SeasonText->SetText(FText::FromString(FString::Printf(
            TEXT("PLAYOFFS  •  ROUND %d"), League.Playoffs.CurrentPlayoffRound + 1)));
    }
    else if (League.Phase == ESeasonPhase::Complete)
    {
        const bool bInOffseason = League.Offseason.CurrentStep != EOffseasonStep::Awards
            || League.Offseason.DraftClass.Num() > 0;
        SeasonText->SetText(FText::FromString(bInOffseason ? TEXT("OFFSEASON") : TEXT("SEASON COMPLETE")));
    }
    else
    {
        SeasonText->SetText(FText::FromString(FString::Printf(
            TEXT("REGULAR SEASON  •  ROUND %d OF 22"), FMath::Min(League.CurrentRound + 1, 22))));
    }
    RecordText->SetText(FText::FromString(FString::Printf(TEXT("%d  -  %d"), Club.Wins, Club.Losses)));
    BalanceText->SetText(FText::FromString(FString::Printf(
        TEXT("$%.1fM"), Club.OperatingBalanceMinorUnits / 100000000.0)));

    const FScheduledGame* NextGame = League.Schedule.FindByPredicate([&Club](const FScheduledGame& Game)
    {
        return !Game.bComplete && (Game.HomeTeamId == Club.TeamId || Game.AwayTeamId == Club.TeamId);
    });
    if (NextGame)
    {
        const FGuid OpponentId = NextGame->HomeTeamId == Club.TeamId ? NextGame->AwayTeamId : NextGame->HomeTeamId;
        const FTeamState* Opponent = League.Teams.FindByPredicate(
            [&OpponentId](const FTeamState& Team) { return Team.TeamId == OpponentId; });
        if (Opponent)
        {
            NextOpponentText->SetText(FText::FromString(FString::Printf(
                TEXT("%s vs %s"), *Club.Nickname, *Opponent->Nickname)));
            MatchupDetailsText->SetText(FText::FromString(FString::Printf(
                TEXT("ROUND %d  •  %s COURT"), NextGame->Round + 1,
                NextGame->HomeTeamId == Club.TeamId ? TEXT("HOME") : TEXT("AWAY"))));
        }
    }
    else if (League.Phase == ESeasonPhase::Playoffs)
    {
        NextOpponentText->SetText(FText::FromString(TEXT("PLAYOFFS IN PROGRESS")));
        const FPlayoffSeries* ActiveSeries = nullptr;
        for (const FPlayoffSeries& Series : League.Playoffs.Series)
        {
            if (Series.bComplete) { continue; }
            if (Series.HigherSeedTeamId == Club.TeamId || Series.LowerSeedTeamId == Club.TeamId)
            {
                ActiveSeries = &Series;
                break;
            }
        }
        if (ActiveSeries)
        {
            const FGuid OppId = ActiveSeries->HigherSeedTeamId == Club.TeamId
                ? ActiveSeries->LowerSeedTeamId : ActiveSeries->HigherSeedTeamId;
            const FTeamState* Opp = League.Teams.FindByPredicate(
                [&OppId](const FTeamState& T) { return T.TeamId == OppId; });
            const int32 ClubWins = ActiveSeries->HigherSeedTeamId == Club.TeamId
                ? ActiveSeries->HigherSeedWins : ActiveSeries->LowerSeedWins;
            const int32 OppWins = ActiveSeries->HigherSeedTeamId == Club.TeamId
                ? ActiveSeries->LowerSeedWins : ActiveSeries->HigherSeedWins;
            MatchupDetailsText->SetText(FText::FromString(FString::Printf(
                TEXT("vs %s  •  SERIES %d-%d"), Opp ? *Opp->Nickname : TEXT("???"), ClubWins, OppWins)));
        }
        else
        {
            MatchupDetailsText->SetText(FText::FromString(TEXT("Eliminated or awaiting next round")));
        }
        SimulateButtonText->SetText(FText::FromString(TEXT("PLAY NEXT GAME  >")));
    }
    else if (League.Phase == ESeasonPhase::Complete)
    {
        const FTeamState* Champion = League.Teams.FindByPredicate(
            [&League](const FTeamState& T) { return T.TeamId == League.Playoffs.ChampionTeamId; });
        NextOpponentText->SetText(FText::FromString(Champion
            ? FString::Printf(TEXT("CHAMPION: %s %s"), *Champion->City.ToUpper(), *Champion->Nickname.ToUpper())
            : TEXT("SEASON COMPLETE")));
        const bool bOffseasonStarted = League.Offseason.CurrentStep != EOffseasonStep::Awards
            || League.Offseason.DraftClass.Num() > 0;
        if (bOffseasonStarted)
        {
            MatchupDetailsText->SetText(FText::FromString(TEXT("Offseason in progress. Use the Offseason tab.")));
            SimulateButtonText->SetText(FText::FromString(TEXT("OFFSEASON  >")));
            SimulateButton->SetIsEnabled(false);
        }
        else
        {
            MatchupDetailsText->SetText(FText::FromString(
                League.Playoffs.ChampionTeamId == Club.TeamId
                ? TEXT("Congratulations! Your rebuild is complete.")
                : TEXT("The season is over. Better luck next year.")));
            SimulateButtonText->SetText(FText::FromString(TEXT("BEGIN OFFSEASON  >")));
            SimulateButton->SetIsEnabled(true);
        }
    }
    else
    {
        NextOpponentText->SetText(FText::FromString(TEXT("REGULAR SEASON COMPLETE")));
        MatchupDetailsText->SetText(FText::FromString(TEXT("Final evaluation ready")));
        SimulateButton->SetIsEnabled(false);
    }

    RosterList->ClearChildren();
    for (int32 Index = 0; Index < FMath::Min(10, Club.Players.Num()); ++Index)
    {
        const FPlayerProfile& Player = Club.Players[Index];
        UBorder* RowBorder = MakeCard(Index % 2 == 0 ? DashboardStyle::CardRaised : DashboardStyle::Card);
        RowBorder->SetPadding(FMargin(12.0f, 7.0f));
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
        RowBorder->SetContent(Row);
        Row->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("%02d"), Index + 1), 9,
            DashboardStyle::Secondary, true))->SetSize(DashboardStyle::Size(0.15f));
        Row->AddChildToHorizontalBox(MakeText(Player.DisplayName, 11, DashboardStyle::Primary, Index < 5))
            ->SetSize(DashboardStyle::Size(1.2f));
        Row->AddChildToHorizontalBox(MakeText(DashboardStyle::Position(Player.Position), 10,
            DashboardStyle::Accent, true))->SetSize(DashboardStyle::Size(0.3f));
        UTextBlock* Overall = MakeText(FString::FromInt(Player.Ratings.Overall()), 11, DashboardStyle::Primary, true);
        Overall->SetJustification(ETextJustify::Right);
        Row->AddChildToHorizontalBox(Overall)->SetSize(DashboardStyle::Size(0.25f));
        RosterList->AddChildToVerticalBox(RowBorder)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 3.0f));
    }

    StandingsList->ClearChildren();
    const TArray<FTeamState> Standings = LeagueSubsystem->GetStandings();
    for (int32 Index = 0; Index < Standings.Num(); ++Index)
    {
        const FTeamState& Team = Standings[Index];
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
        Row->AddChildToHorizontalBox(MakeText(FString::FromInt(Index + 1), 9,
            Index < 4 ? DashboardStyle::Accent : DashboardStyle::Secondary, true))
            ->SetSize(DashboardStyle::Size(0.18f));
        Row->AddChildToHorizontalBox(MakeText(Team.Nickname, 10,
            Team.TeamId == Club.TeamId ? DashboardStyle::Primary : DashboardStyle::Secondary,
            Team.TeamId == Club.TeamId))->SetSize(DashboardStyle::Size(1.0f));
        UTextBlock* TeamRecord = MakeText(FString::Printf(TEXT("%d-%d"), Team.Wins, Team.Losses),
            10, DashboardStyle::Primary, true);
        TeamRecord->SetJustification(ETextJustify::Right);
        Row->AddChildToHorizontalBox(TeamRecord)->SetSize(DashboardStyle::Size(0.35f));
        StandingsList->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.0f, 4.0f));
    }

    RefreshRosterScreen(League, Club);
    RefreshScheduleScreen(League, Club);
    RefreshStandingsScreen(Standings, Club.TeamId);
    RefreshScoutingScreen(League, Club);
    RefreshTrainingScreen(Club);
    RefreshTradeScreen(League, Club);
    RefreshPlayoffScreen(League, Club);
    RefreshResultsScreen(LastRoundResults, League, Club);
    RefreshAwardsScreen(League, Club);
    RefreshOffseasonScreen(League, Club);
}

void UManagementDashboardWidget::RefreshRosterScreen(const FLeagueState& League, const FTeamState& Club)
{
    RosterDetailList->ClearChildren();
    UButton* AutoButton = WidgetTree->ConstructWidget<UButton>();
    AutoButton->SetBackgroundColor(DashboardStyle::Accent);
    AutoButton->SetContent(MakeText(TEXT("AUTO-BUILD BEST ROTATION"), 11, DashboardStyle::Background, true));
    AutoButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleAutoRotation);
    RosterDetailList->AddChildToVerticalBox(AutoButton)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));

    for (int32 Index = 0; Index < Club.Rotation.OrderedPlayers.Num(); ++Index)
    {
        const FGuid PlayerId = Club.Rotation.OrderedPlayers[Index];
        const FPlayerProfile* Player = Club.Players.FindByPredicate(
            [&PlayerId](const FPlayerProfile& Candidate) { return Candidate.PlayerId == PlayerId; });
        const FAthleteState* State = Club.PlayerStates.FindByPredicate(
            [&PlayerId](const FAthleteState& Candidate) { return Candidate.PlayerId == PlayerId; });
        if (!Player || !State) { continue; }
        UBorder* RowBorder = MakeCard(Index % 2 == 0 ? DashboardStyle::CardRaised : DashboardStyle::Card);
        RowBorder->SetPadding(FMargin(12.0f, 6.0f));
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
        RowBorder->SetContent(Row);
        Row->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("%02d"), Index + 1), 9,
            DashboardStyle::Secondary, true))->SetSize(DashboardStyle::Size(0.12f));
        Row->AddChildToHorizontalBox(MakeText(Player->DisplayName, 11, DashboardStyle::Primary, Index < 5))
            ->SetSize(DashboardStyle::Size(1.2f));
        Row->AddChildToHorizontalBox(MakeText(DashboardStyle::Position(Player->Position), 10,
            DashboardStyle::Accent, true))->SetSize(DashboardStyle::Size(0.22f));
        Row->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("OVR %d"), Player->Ratings.Overall()), 10,
            DashboardStyle::Primary, true))->SetSize(DashboardStyle::Size(0.35f));
        Row->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("FIT %d"), State->Fitness), 10,
            State->Fitness >= 70 ? DashboardStyle::Success : DashboardStyle::Accent, true))
            ->SetSize(DashboardStyle::Size(0.35f));
        const FSeasonStats* Stats = League.SeasonStats.FindByPredicate(
            [&PlayerId](const FSeasonStats& S) { return S.PlayerId == PlayerId; });
        if (Stats && Stats->GamesPlayed > 0)
        {
            Row->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("%.1f/%.1f/%.1f"),
                Stats->PPG(), Stats->RPG(), Stats->APG()), 9, DashboardStyle::Secondary))
                ->SetSize(DashboardStyle::Size(0.55f));
        }
        else
        {
            Row->AddChildToHorizontalBox(MakeText(TEXT("—/—/—"), 9, DashboardStyle::Secondary))
                ->SetSize(DashboardStyle::Size(0.55f));
        }
        UTextBlock* Minutes = MakeText(FString::Printf(TEXT("%d MIN"), Club.Rotation.TargetMinutes.FindRef(PlayerId)),
            10, DashboardStyle::Secondary, true);
        Minutes->SetJustification(ETextJustify::Right);
        Row->AddChildToHorizontalBox(Minutes)->SetSize(DashboardStyle::Size(0.32f));
        RosterDetailList->AddChildToVerticalBox(RowBorder)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 2.0f));
    }
}

void UManagementDashboardWidget::RefreshScheduleScreen(const FLeagueState& League, const FTeamState& Club)
{
    ScheduleList->ClearChildren();
    int32 Added = 0;
    const int32 StartRound = FMath::Max(0, League.CurrentRound - 3);
    const int32 EndRound = FMath::Min(21, StartRound + 11);
    for (const FScheduledGame& Game : League.Schedule)
    {
        if (Game.Round < StartRound || Game.Round > EndRound
            || (Game.HomeTeamId != Club.TeamId && Game.AwayTeamId != Club.TeamId)) { continue; }
        const FGuid OpponentId = Game.HomeTeamId == Club.TeamId ? Game.AwayTeamId : Game.HomeTeamId;
        const FTeamState* Opponent = League.Teams.FindByPredicate(
            [&OpponentId](const FTeamState& Team) { return Team.TeamId == OpponentId; });
        if (!Opponent) { continue; }
        const bool bHome = Game.HomeTeamId == Club.TeamId;
        FString Result = TEXT("UPCOMING");
        FLinearColor ResultColor = DashboardStyle::Secondary;
        if (Game.bComplete)
        {
            const int32 ClubScore = bHome ? Game.HomeScore : Game.AwayScore;
            const int32 OpponentScore = bHome ? Game.AwayScore : Game.HomeScore;
            Result = FString::Printf(TEXT("%s  %d-%d"), ClubScore > OpponentScore ? TEXT("W") : TEXT("L"), ClubScore, OpponentScore);
            ResultColor = ClubScore > OpponentScore ? DashboardStyle::Success : FLinearColor(0.95f, 0.32f, 0.28f, 1.0f);
        }
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
        Row->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("R%02d"), Game.Round + 1), 10,
            DashboardStyle::Secondary, true))->SetSize(DashboardStyle::Size(0.22f));
        Row->AddChildToHorizontalBox(MakeText(bHome ? TEXT("HOME") : TEXT("AWAY"), 9,
            DashboardStyle::Accent, true))->SetSize(DashboardStyle::Size(0.28f));
        Row->AddChildToHorizontalBox(MakeText(Opponent->City + TEXT(" ") + Opponent->Nickname, 11,
            DashboardStyle::Primary, !Game.bComplete))->SetSize(DashboardStyle::Size(1.0f));
        UTextBlock* ResultText = MakeText(Result, 10, ResultColor, true);
        ResultText->SetJustification(ETextJustify::Right);
        Row->AddChildToHorizontalBox(ResultText)->SetSize(DashboardStyle::Size(0.4f));
        ScheduleList->AddChildToVerticalBox(Row)->SetPadding(FMargin(4.0f, 7.0f));
        Added++;
    }
    if (Added == 0)
    {
        ScheduleList->AddChildToVerticalBox(MakeText(TEXT("No fixtures in this range."), 11, DashboardStyle::Secondary));
    }
}

void UManagementDashboardWidget::RefreshStandingsScreen(const TArray<FTeamState>& Standings, const FGuid& ClubId)
{
    StandingsDetailList->ClearChildren();
    for (int32 Index = 0; Index < Standings.Num(); ++Index)
    {
        const FTeamState& Team = Standings[Index];
        UBorder* RowBorder = MakeCard(Team.TeamId == ClubId ? DashboardStyle::AccentSoft
            : Index % 2 == 0 ? DashboardStyle::CardRaised : DashboardStyle::Card);
        RowBorder->SetPadding(FMargin(12.0f, 6.0f));
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
        RowBorder->SetContent(Row);
        Row->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("%02d"), Index + 1), 10,
            Index < 4 ? DashboardStyle::Accent : DashboardStyle::Secondary, true))
            ->SetSize(DashboardStyle::Size(0.14f));
        Row->AddChildToHorizontalBox(MakeText(Team.City + TEXT(" ") + Team.Nickname, 11,
            DashboardStyle::Primary, Team.TeamId == ClubId))->SetSize(DashboardStyle::Size(1.2f));
        Row->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("%d-%d"), Team.Wins, Team.Losses), 10,
            DashboardStyle::Primary, true))->SetSize(DashboardStyle::Size(0.32f));
        UTextBlock* Differential = MakeText(FString::Printf(TEXT("%+d DIFF"), Team.PointsFor - Team.PointsAgainst),
            10, DashboardStyle::Secondary, true);
        Differential->SetJustification(ETextJustify::Right);
        Row->AddChildToHorizontalBox(Differential)->SetSize(DashboardStyle::Size(0.4f));
        StandingsDetailList->AddChildToVerticalBox(RowBorder)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 2.0f));
    }
}

void UManagementDashboardWidget::RefreshScoutingScreen(const FLeagueState& League, const FTeamState& Club)
{
    ScoutingList->ClearChildren();
    RecommendedScoutPlayerId.Invalidate();
    int32 Active = 0;
    for (const FScoutingAssignment& Assignment : League.ScoutingAssignments)
    {
        if (Assignment.RequestedByTeamId != Club.TeamId || Assignment.bComplete) { continue; }
        Active++;
        const FPlayerProfile* Player = nullptr;
        for (const FTeamState& Team : League.Teams)
        {
            Player = Team.Players.FindByPredicate([&Assignment](const FPlayerProfile& Candidate)
            { return Candidate.PlayerId == Assignment.PlayerId; });
            if (Player) { break; }
        }
        if (Player)
        {
            ScoutingList->AddChildToVerticalBox(MakeText(FString::Printf(TEXT("IN PROGRESS  •  %s  •  DUE ROUND %d"),
                *Player->DisplayName, Assignment.CompletionRound + 1), 10, DashboardStyle::Accent, true))
                ->SetPadding(FMargin(0.0f, 3.0f));
        }
    }
    ScoutingStatusText = MakeText(FString::Printf(TEXT("SCOUTING CAPACITY  %d / 3"), Active), 10,
        Active < 3 ? DashboardStyle::Success : DashboardStyle::Secondary, true);
    ScoutingList->AddChildToVerticalBox(ScoutingStatusText)->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 10.0f));

    const FPlayerProfile* BestCandidate = nullptr;
    const FTeamState* CandidateTeam = nullptr;
    for (const FTeamState& Team : League.Teams)
    {
        if (Team.TeamId == Club.TeamId) { continue; }
        for (const FPlayerProfile& Player : Team.Players)
        {
            bool bKnown = false;
            for (const FScoutingAssignment& Assignment : League.ScoutingAssignments)
            {
                bKnown |= Assignment.RequestedByTeamId == Club.TeamId && Assignment.PlayerId == Player.PlayerId;
            }
            for (const FScoutingReport& Report : League.ScoutingReports)
            {
                bKnown |= Report.RequestedByTeamId == Club.TeamId && Report.PlayerId == Player.PlayerId;
            }
            if (!bKnown && (!BestCandidate || Player.Ratings.Overall() > BestCandidate->Ratings.Overall()))
            {
                BestCandidate = &Player;
                CandidateTeam = &Team;
            }
        }
    }
    if (BestCandidate && CandidateTeam)
    {
        RecommendedScoutPlayerId = BestCandidate->PlayerId;
        UBorder* Recommendation = MakeCard(DashboardStyle::CardRaised);
        UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
        Recommendation->SetContent(Content);
        Content->AddChildToVerticalBox(MakeText(TEXT("RECRUITMENT TEAM RECOMMENDATION"), 9,
            DashboardStyle::Accent, true));
        Content->AddChildToVerticalBox(MakeText(BestCandidate->DisplayName, 17,
            DashboardStyle::Primary, true))->SetPadding(FMargin(0.0f, 5.0f, 0.0f, 0.0f));
        Content->AddChildToVerticalBox(MakeText(FString::Printf(TEXT("%s  •  %s %s  •  OVERALL ??  •  POTENTIAL ??"),
            *DashboardStyle::Position(BestCandidate->Position), *CandidateTeam->City, *CandidateTeam->Nickname),
            10, DashboardStyle::Secondary));
        ScoutingList->AddChildToVerticalBox(Recommendation)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
        UButton* AssignButton = WidgetTree->ConstructWidget<UButton>();
        AssignButton->SetBackgroundColor(DashboardStyle::Accent);
        AssignButton->SetContent(MakeText(TEXT("ASSIGN SCOUT"), 11, DashboardStyle::Background, true));
        AssignButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleAssignScout);
        AssignButton->SetIsEnabled(Active < 3);
        ScoutingList->AddChildToVerticalBox(AssignButton)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
    }
    for (const FScoutingReport& Report : League.ScoutingReports)
    {
        if (Report.RequestedByTeamId != Club.TeamId) { continue; }
        const FPlayerProfile* Player = nullptr;
        for (const FTeamState& Team : League.Teams)
        {
            Player = Team.Players.FindByPredicate([&Report](const FPlayerProfile& Candidate)
            { return Candidate.PlayerId == Report.PlayerId; });
            if (Player) { break; }
        }
        if (Player)
        {
            ScoutingList->AddChildToVerticalBox(MakeText(FString::Printf(
                TEXT("REPORT  •  %s  •  OVR %d-%d  •  POT %d-%d"), *Player->DisplayName,
                Report.OverallMin, Report.OverallMax, Report.PotentialMin, Report.PotentialMax),
                10, DashboardStyle::Primary, true))->SetPadding(FMargin(0.0f, 4.0f));
        }
    }
}

void UManagementDashboardWidget::RefreshTrainingScreen(const FTeamState& Club)
{
    TrainingList->ClearChildren();
    const UEnum* FocusEnum = StaticEnum<ETrainingFocus>();
    const UEnum* IntensityEnum = StaticEnum<ETrainingIntensity>();
    TrainingPlanText = MakeText(FString::Printf(TEXT("CURRENT PLAN  •  %s  •  %s INTENSITY"),
        *FocusEnum->GetDisplayNameTextByValue(static_cast<int64>(Club.TrainingPlan.Focus)).ToString().ToUpper(),
        *IntensityEnum->GetDisplayNameTextByValue(static_cast<int64>(Club.TrainingPlan.Intensity)).ToString().ToUpper()),
        12, DashboardStyle::Accent, true);
    TrainingList->AddChildToVerticalBox(TrainingPlanText)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
    auto AddTrainingButton = [this](const FString& Label, const FName HandlerName)
    {
        UButton* Button = WidgetTree->ConstructWidget<UButton>();
        Button->SetBackgroundColor(DashboardStyle::CardRaised);
        Button->SetContent(MakeText(Label, 11, DashboardStyle::Primary, true));
        FScriptDelegate Handler;
        Handler.BindUFunction(this, HandlerName);
        Button->OnClicked.Add(Handler);
        TrainingList->AddChildToVerticalBox(Button)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
    };
    AddTrainingButton(TEXT("BALANCED DEVELOPMENT"), GET_FUNCTION_NAME_CHECKED(UManagementDashboardWidget, SetTrainingBalanced));
    AddTrainingButton(TEXT("SHOOTING FOCUS"), GET_FUNCTION_NAME_CHECKED(UManagementDashboardWidget, SetTrainingShooting));
    AddTrainingButton(TEXT("DEFENSIVE FOCUS"), GET_FUNCTION_NAME_CHECKED(UManagementDashboardWidget, SetTrainingDefense));
    AddTrainingButton(TEXT("CONDITIONING FOCUS"), GET_FUNCTION_NAME_CHECKED(UManagementDashboardWidget, SetTrainingConditioning));
    AddTrainingButton(TEXT("RECOVERY INTENSITY  •  -FATIGUE / NO GROWTH"), GET_FUNCTION_NAME_CHECKED(UManagementDashboardWidget, SetTrainingRecovery));
    AddTrainingButton(TEXT("HIGH INTENSITY  •  +GROWTH / +INJURY RISK"), GET_FUNCTION_NAME_CHECKED(UManagementDashboardWidget, SetTrainingHigh));
}

void UManagementDashboardWidget::SetScreen(int32 Index)
{
    if (ScreenSwitcher) { ScreenSwitcher->SetActiveWidgetIndex(Index); }
}

void UManagementDashboardWidget::ShowOverview() { SetScreen(0); }
void UManagementDashboardWidget::ShowRoster() { SetScreen(1); }
void UManagementDashboardWidget::ShowSchedule() { SetScreen(2); }
void UManagementDashboardWidget::ShowStandings() { SetScreen(3); }
void UManagementDashboardWidget::ShowScouting() { SetScreen(4); }
void UManagementDashboardWidget::ShowTraining() { SetScreen(5); }
void UManagementDashboardWidget::ShowTrades() { SetScreen(6); }
void UManagementDashboardWidget::ShowPlayoffs() { SetScreen(7); }
void UManagementDashboardWidget::ShowAwards() { SetScreen(9); }
void UManagementDashboardWidget::ShowOffseason() { SetScreen(10); }

void UManagementDashboardWidget::HandleAutoRotation()
{
    ULeagueGameSubsystem* Subsystem = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>();
    const FLeagueState League = Subsystem->GetLeague();
    if (League.Teams.Num() == 0) { return; }
    FString Error;
    if (Subsystem->AutoBuildRotation(League.Teams[0].TeamId, Error)) { RefreshDashboard(); SetScreen(1); }
}

void UManagementDashboardWidget::HandleAssignScout()
{
    ULeagueGameSubsystem* Subsystem = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>();
    const FLeagueState League = Subsystem->GetLeague();
    if (League.Teams.Num() == 0 || !RecommendedScoutPlayerId.IsValid()) { return; }
    FString Error;
    if (Subsystem->AssignScout(League.Teams[0].TeamId, RecommendedScoutPlayerId, Error))
    { RefreshDashboard(); SetScreen(4); }
    else if (ScoutingStatusText) { ScoutingStatusText->SetText(FText::FromString(Error)); }
}

void UManagementDashboardWidget::ApplyTrainingPlan(ETrainingFocus Focus, ETrainingIntensity Intensity)
{
    ULeagueGameSubsystem* Subsystem = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>();
    const FLeagueState League = Subsystem->GetLeague();
    if (League.Teams.Num() == 0) { return; }
    FString Error;
    if (Subsystem->SetTrainingPlan(League.Teams[0].TeamId, Focus, Intensity, Error))
    { RefreshDashboard(); SetScreen(5); }
}

void UManagementDashboardWidget::RefreshTradeScreen(const FLeagueState& League, const FTeamState& Club)
{
    TradeList->ClearChildren();
    SelectedTradePlayerId.Invalidate();
    SelectedTradeTeamId.Invalidate();

    const bool bOpen = League.Phase == ESeasonPhase::RegularSeason
        && League.CurrentRound < League.TradeDeadlineRound;
    TradeList->AddChildToVerticalBox(MakeText(
        bOpen ? FString::Printf(TEXT("TRADE WINDOW OPEN  •  DEADLINE ROUND %d"), League.TradeDeadlineRound)
              : TEXT("TRADE WINDOW CLOSED"),
        11, bOpen ? DashboardStyle::Success : DashboardStyle::Secondary, true))
        ->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));

    if (bOpen)
    {
        TradeList->AddChildToVerticalBox(MakeText(TEXT("AVAILABLE TARGETS"), 10,
            DashboardStyle::Accent, true))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

        const FPlayerProfile* BestTarget = nullptr;
        const FTeamState* TargetTeam = nullptr;
        for (const FTeamState& Team : League.Teams)
        {
            if (Team.TeamId == Club.TeamId) { continue; }
            for (const FPlayerProfile& Player : Team.Players)
            {
                if (!BestTarget || Player.Ratings.Overall() > BestTarget->Ratings.Overall())
                {
                    BestTarget = &Player;
                    TargetTeam = &Team;
                }
            }
        }
        if (BestTarget && TargetTeam)
        {
            SelectedTradePlayerId = BestTarget->PlayerId;
            SelectedTradeTeamId = TargetTeam->TeamId;
            UBorder* Card = MakeCard(DashboardStyle::CardRaised);
            UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
            Card->SetContent(Content);
            Content->AddChildToVerticalBox(MakeText(BestTarget->DisplayName, 16,
                DashboardStyle::Primary, true));
            Content->AddChildToVerticalBox(MakeText(FString::Printf(
                TEXT("%s  •  %s %s  •  OVR %d  •  AGE %d"),
                *DashboardStyle::Position(BestTarget->Position),
                *TargetTeam->City, *TargetTeam->Nickname,
                BestTarget->Ratings.Overall(), BestTarget->Age),
                10, DashboardStyle::Secondary))->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
            TradeList->AddChildToVerticalBox(Card)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));

            if (Club.Players.Num() > 10)
            {
                const FPlayerProfile* Weakest = &Club.Players[0];
                for (const FPlayerProfile& P : Club.Players)
                {
                    if (P.Ratings.Overall() < Weakest->Ratings.Overall()) { Weakest = &P; }
                }
                TradeList->AddChildToVerticalBox(MakeText(FString::Printf(
                    TEXT("OFFERING: %s (OVR %d)"), *Weakest->DisplayName, Weakest->Ratings.Overall()),
                    10, DashboardStyle::Secondary))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

                UButton* TradeButton = WidgetTree->ConstructWidget<UButton>();
                TradeButton->SetBackgroundColor(DashboardStyle::Accent);
                TradeButton->SetContent(MakeText(TEXT("PROPOSE TRADE"), 11,
                    DashboardStyle::Background, true));
                TradeButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleProposeTrade);
                TradeList->AddChildToVerticalBox(TradeButton)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
            }
        }
    }

    TradeList->AddChildToVerticalBox(MakeText(TEXT("TRADE HISTORY"), 10,
        DashboardStyle::Accent, true))->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 8.0f));
    if (League.TradeHistory.Num() == 0)
    {
        TradeList->AddChildToVerticalBox(MakeText(TEXT("No trades this season."), 10,
            DashboardStyle::Secondary));
    }
    for (int32 Index = League.TradeHistory.Num() - 1; Index >= FMath::Max(0, League.TradeHistory.Num() - 10); --Index)
    {
        const FTradeOffer& Trade = League.TradeHistory[Index];
        const FString StatusStr = Trade.Status == ETradeStatus::Accepted ? TEXT("ACCEPTED")
            : Trade.Status == ETradeStatus::Rejected ? TEXT("REJECTED") : TEXT("EXPIRED");
        const FLinearColor StatusColor = Trade.Status == ETradeStatus::Accepted
            ? DashboardStyle::Success : FLinearColor(0.95f, 0.32f, 0.28f, 1.0f);
        TradeList->AddChildToVerticalBox(MakeText(FString::Printf(TEXT("R%d  •  %s  •  %d for %d players"),
            Trade.ProposedRound + 1, *StatusStr, Trade.Outgoing.Num(), Trade.Incoming.Num()),
            10, StatusColor, true))->SetPadding(FMargin(0.0f, 3.0f));
    }
}

void UManagementDashboardWidget::RefreshPlayoffScreen(const FLeagueState& League, const FTeamState& Club)
{
    PlayoffList->ClearChildren();
    if (League.Phase == ESeasonPhase::RegularSeason)
    {
        PlayoffList->AddChildToVerticalBox(MakeText(TEXT("Playoffs begin after the regular season."),
            12, DashboardStyle::Secondary));
        return;
    }

    if (League.Playoffs.ChampionTeamId.IsValid())
    {
        const FTeamState* Champ = League.Teams.FindByPredicate(
            [&League](const FTeamState& T) { return T.TeamId == League.Playoffs.ChampionTeamId; });
        PlayoffList->AddChildToVerticalBox(MakeText(
            FString::Printf(TEXT("CHAMPION: %s %s"),
            Champ ? *Champ->City.ToUpper() : TEXT("???"),
            Champ ? *Champ->Nickname.ToUpper() : TEXT("")),
            22, DashboardStyle::Accent, true))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
    }

    const FString RoundNames[] = { TEXT("QUARTERFINALS"), TEXT("SEMIFINALS"), TEXT("FINALS") };
    for (int32 Round = 0; Round <= League.Playoffs.CurrentPlayoffRound; ++Round)
    {
        const FString& RoundLabel = Round < 3 ? RoundNames[Round] : FString::Printf(TEXT("ROUND %d"), Round + 1);
        PlayoffList->AddChildToVerticalBox(MakeText(RoundLabel, 12, DashboardStyle::Accent, true))
            ->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 6.0f));

        for (const FPlayoffSeries& Series : League.Playoffs.Series)
        {
            if (Series.PlayoffRound != Round) { continue; }
            const FTeamState* High = League.Teams.FindByPredicate(
                [&Series](const FTeamState& T) { return T.TeamId == Series.HigherSeedTeamId; });
            const FTeamState* Low = League.Teams.FindByPredicate(
                [&Series](const FTeamState& T) { return T.TeamId == Series.LowerSeedTeamId; });
            const bool bClubSeries = Series.HigherSeedTeamId == Club.TeamId
                || Series.LowerSeedTeamId == Club.TeamId;
            UBorder* Row = MakeCard(bClubSeries ? DashboardStyle::AccentSoft : DashboardStyle::CardRaised);
            Row->SetPadding(FMargin(12.0f, 8.0f));
            UHorizontalBox* Content = WidgetTree->ConstructWidget<UHorizontalBox>();
            Row->SetContent(Content);
            Content->AddChildToHorizontalBox(MakeText(
                High ? High->Nickname : TEXT("???"), 12, DashboardStyle::Primary, true))
                ->SetSize(DashboardStyle::Size(1.0f));
            Content->AddChildToHorizontalBox(MakeText(
                FString::Printf(TEXT("%d  -  %d"), Series.HigherSeedWins, Series.LowerSeedWins),
                14, DashboardStyle::Accent, true))
                ->SetSize(DashboardStyle::Size(0.4f, ESlateSizeRule::Automatic));
            UTextBlock* LowText = MakeText(Low ? Low->Nickname : TEXT("???"), 12, DashboardStyle::Primary, true);
            LowText->SetJustification(ETextJustify::Right);
            Content->AddChildToHorizontalBox(LowText)->SetSize(DashboardStyle::Size(1.0f));
            PlayoffList->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
        }
    }
}

void UManagementDashboardWidget::RefreshResultsScreen(const TArray<FMatchResult>& Results,
    const FLeagueState& League, const FTeamState& Club)
{
    ResultsList->ClearChildren();
    if (Results.Num() == 0)
    {
        ResultsList->AddChildToVerticalBox(MakeText(TEXT("No results yet. Simulate a round to see outcomes."),
            12, DashboardStyle::Secondary));
        return;
    }

    for (const FMatchResult& Result : Results)
    {
        const FScheduledGame* Game = League.Schedule.FindByPredicate(
            [&Result](const FScheduledGame& G) { return G.GameId == Result.GameId; });
        if (!Game) { continue; }
        const FTeamState* Home = League.Teams.FindByPredicate(
            [&Game](const FTeamState& T) { return T.TeamId == Game->HomeTeamId; });
        const FTeamState* Away = League.Teams.FindByPredicate(
            [&Game](const FTeamState& T) { return T.TeamId == Game->AwayTeamId; });
        if (!Home || !Away) { continue; }

        const bool bClubGame = Game->HomeTeamId == Club.TeamId || Game->AwayTeamId == Club.TeamId;
        UBorder* GameCard = MakeCard(bClubGame ? DashboardStyle::AccentSoft : DashboardStyle::CardRaised);
        GameCard->SetPadding(FMargin(14.0f, 10.0f));
        UVerticalBox* GameContent = WidgetTree->ConstructWidget<UVerticalBox>();
        GameCard->SetContent(GameContent);

        UHorizontalBox* ScoreLine = WidgetTree->ConstructWidget<UHorizontalBox>();
        ScoreLine->AddChildToHorizontalBox(MakeText(Home->Nickname, 14, DashboardStyle::Primary, true))
            ->SetSize(DashboardStyle::Size(1.0f));
        ScoreLine->AddChildToHorizontalBox(MakeText(
            FString::Printf(TEXT("%d  -  %d"), Result.HomeScore, Result.AwayScore),
            16, DashboardStyle::Accent, true))
            ->SetSize(DashboardStyle::Size(0.5f, ESlateSizeRule::Automatic));
        UTextBlock* AwayName = MakeText(Away->Nickname, 14, DashboardStyle::Primary, true);
        AwayName->SetJustification(ETextJustify::Right);
        ScoreLine->AddChildToHorizontalBox(AwayName)->SetSize(DashboardStyle::Size(1.0f));
        GameContent->AddChildToVerticalBox(ScoreLine);

        if (Result.PeriodsPlayed > 4)
        {
            GameContent->AddChildToVerticalBox(MakeText(
                FString::Printf(TEXT("OVERTIME  •  %d PERIODS"), Result.PeriodsPlayed),
                9, DashboardStyle::Accent, true))->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
        }

        if (bClubGame)
        {
            const TArray<FPlayerBoxScore>& Box = Game->HomeTeamId == Club.TeamId
                ? Result.HomeBoxScore : Result.AwayBoxScore;
            GameContent->AddChildToVerticalBox(MakeText(TEXT(""), 6, DashboardStyle::Background))
                ->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));

            UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
            Header->AddChildToHorizontalBox(MakeText(TEXT("PLAYER"), 8, DashboardStyle::Secondary, true))
                ->SetSize(DashboardStyle::Size(1.2f));
            Header->AddChildToHorizontalBox(MakeText(TEXT("PTS"), 8, DashboardStyle::Secondary, true))
                ->SetSize(DashboardStyle::Size(0.3f));
            Header->AddChildToHorizontalBox(MakeText(TEXT("REB"), 8, DashboardStyle::Secondary, true))
                ->SetSize(DashboardStyle::Size(0.3f));
            Header->AddChildToHorizontalBox(MakeText(TEXT("AST"), 8, DashboardStyle::Secondary, true))
                ->SetSize(DashboardStyle::Size(0.3f));
            Header->AddChildToHorizontalBox(MakeText(TEXT("STL"), 8, DashboardStyle::Secondary, true))
                ->SetSize(DashboardStyle::Size(0.3f));
            Header->AddChildToHorizontalBox(MakeText(TEXT("BLK"), 8, DashboardStyle::Secondary, true))
                ->SetSize(DashboardStyle::Size(0.3f));
            UTextBlock* FGLabel = MakeText(TEXT("FG"), 8, DashboardStyle::Secondary, true);
            FGLabel->SetJustification(ETextJustify::Right);
            Header->AddChildToHorizontalBox(FGLabel)->SetSize(DashboardStyle::Size(0.45f));
            GameContent->AddChildToVerticalBox(Header)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));

            for (const FPlayerBoxScore& PlayerBox : Box)
            {
                if (PlayerBox.Points == 0 && PlayerBox.Rebounds == 0 && PlayerBox.Assists == 0) { continue; }
                const FPlayerProfile* Player = Club.Players.FindByPredicate(
                    [&PlayerBox](const FPlayerProfile& P) { return P.PlayerId == PlayerBox.PlayerId; });
                UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
                Row->AddChildToHorizontalBox(MakeText(Player ? Player->DisplayName : TEXT("???"), 9,
                    DashboardStyle::Primary))->SetSize(DashboardStyle::Size(1.2f));
                Row->AddChildToHorizontalBox(MakeText(FString::FromInt(PlayerBox.Points), 9,
                    DashboardStyle::Primary, true))->SetSize(DashboardStyle::Size(0.3f));
                Row->AddChildToHorizontalBox(MakeText(FString::FromInt(PlayerBox.Rebounds), 9,
                    DashboardStyle::Primary))->SetSize(DashboardStyle::Size(0.3f));
                Row->AddChildToHorizontalBox(MakeText(FString::FromInt(PlayerBox.Assists), 9,
                    DashboardStyle::Primary))->SetSize(DashboardStyle::Size(0.3f));
                Row->AddChildToHorizontalBox(MakeText(FString::FromInt(PlayerBox.Steals), 9,
                    DashboardStyle::Primary))->SetSize(DashboardStyle::Size(0.3f));
                Row->AddChildToHorizontalBox(MakeText(FString::FromInt(PlayerBox.Blocks), 9,
                    DashboardStyle::Primary))->SetSize(DashboardStyle::Size(0.3f));
                UTextBlock* FGText = MakeText(FString::Printf(TEXT("%d/%d"),
                    PlayerBox.FieldGoalsMade, PlayerBox.FieldGoalsAttempted), 9, DashboardStyle::Secondary);
                FGText->SetJustification(ETextJustify::Right);
                Row->AddChildToHorizontalBox(FGText)->SetSize(DashboardStyle::Size(0.45f));
                GameContent->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.0f, 2.0f));
            }

            const FMatchSnapshot* Snap = LastRoundSnapshots.FindByPredicate(
                [&Result](const FMatchSnapshot& S) { return S.GameId == Result.GameId; });
            if (Snap)
            {
                TArray<FCommentaryLine> Highlights = FCommentaryService::GenerateHighlights(Result, *Snap, 5);
                if (Highlights.Num() > 0)
                {
                    GameContent->AddChildToVerticalBox(MakeText(TEXT("HIGHLIGHTS"), 8,
                        DashboardStyle::Accent, true))->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 4.0f));
                    for (const FCommentaryLine& Line : Highlights)
                    {
                        UTextBlock* CommentaryText = MakeText(Line.Text, 9, DashboardStyle::Secondary);
                        CommentaryText->SetAutoWrapText(true);
                        GameContent->AddChildToVerticalBox(CommentaryText)->SetPadding(FMargin(0.0f, 2.0f));
                    }
                }
            }
        }
        ResultsList->AddChildToVerticalBox(GameCard)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
    }
}

void UManagementDashboardWidget::HandleProposeTrade()
{
    ULeagueGameSubsystem* Subsystem = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>();
    const FLeagueState League = Subsystem->GetLeague();
    if (League.Teams.Num() == 0 || !SelectedTradePlayerId.IsValid() || !SelectedTradeTeamId.IsValid()) { return; }
    const FTeamState& Club = League.Teams[0];
    const FPlayerProfile* Weakest = &Club.Players[0];
    for (const FPlayerProfile& P : Club.Players)
    {
        if (P.Ratings.Overall() < Weakest->Ratings.Overall()) { Weakest = &P; }
    }
    FString Error;
    TArray<FGuid> Outgoing = { Weakest->PlayerId };
    TArray<FGuid> Incoming = { SelectedTradePlayerId };
    if (Subsystem->ProposeTrade(Club.TeamId, Outgoing, SelectedTradeTeamId, Incoming, Error))
    {
        RefreshDashboard();
        SetScreen(6);
        StatusText->SetText(FText::FromString(TEXT("Trade accepted and executed!")));
        StatusText->SetColorAndOpacity(FSlateColor(DashboardStyle::Success));
    }
    else
    {
        RefreshDashboard();
        SetScreen(6);
        StatusText->SetText(FText::FromString(Error));
        StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.32f, 0.28f, 1.0f)));
    }
}

void UManagementDashboardWidget::SetTrainingBalanced()
{ const FLeagueState League = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>()->GetLeague(); if (League.Teams.Num()) { ApplyTrainingPlan(ETrainingFocus::Balanced, League.Teams[0].TrainingPlan.Intensity); } }
void UManagementDashboardWidget::SetTrainingShooting()
{ const FLeagueState League = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>()->GetLeague(); if (League.Teams.Num()) { ApplyTrainingPlan(ETrainingFocus::Shooting, League.Teams[0].TrainingPlan.Intensity); } }
void UManagementDashboardWidget::SetTrainingDefense()
{ const FLeagueState League = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>()->GetLeague(); if (League.Teams.Num()) { ApplyTrainingPlan(ETrainingFocus::Defense, League.Teams[0].TrainingPlan.Intensity); } }
void UManagementDashboardWidget::SetTrainingConditioning()
{ const FLeagueState League = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>()->GetLeague(); if (League.Teams.Num()) { ApplyTrainingPlan(ETrainingFocus::Conditioning, League.Teams[0].TrainingPlan.Intensity); } }
void UManagementDashboardWidget::SetTrainingRecovery()
{ const FLeagueState League = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>()->GetLeague(); if (League.Teams.Num()) { ApplyTrainingPlan(League.Teams[0].TrainingPlan.Focus, ETrainingIntensity::Recovery); } }
void UManagementDashboardWidget::SetTrainingHigh()
{ const FLeagueState League = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>()->GetLeague(); if (League.Teams.Num()) { ApplyTrainingPlan(League.Teams[0].TrainingPlan.Focus, ETrainingIntensity::High); } }

void UManagementDashboardWidget::HandleSimulateRound()
{
    ULeagueGameSubsystem* LeagueSubsystem = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>();
    if (!LeagueSubsystem) { return; }
    FString Error;

    if (LeagueSubsystem->GetSeasonPhase() == ESeasonPhase::Complete)
    {
        if (LeagueSubsystem->StartOffseason(Error))
        {
            RefreshDashboard();
            StatusText->SetText(FText::FromString(TEXT("Offseason has begun. Step through each phase.")));
            StatusText->SetColorAndOpacity(FSlateColor(DashboardStyle::Success));
            SetScreen(10);
        }
        else
        {
            StatusText->SetText(FText::FromString(Error));
            StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.32f, 0.28f, 1.0f)));
        }
        return;
    }

    TArray<FMatchResult> Results;
    bool bSuccess = false;
    if (LeagueSubsystem->GetSeasonPhase() == ESeasonPhase::Playoffs)
    {
        bSuccess = LeagueSubsystem->AdvancePlayoffs(Results, Error);
    }
    else
    {
        bSuccess = LeagueSubsystem->AdvanceCurrentRound(Results, Error);
    }

    if (bSuccess)
    {
        LastRoundResults = Results;
        LastRoundSnapshots.Empty();
        const FLeagueState PostLeague = LeagueSubsystem->GetLeague();
        for (const FMatchResult& Result : Results)
        {
            const FScheduledGame* Game = PostLeague.Schedule.FindByPredicate(
                [&Result](const FScheduledGame& G) { return G.GameId == Result.GameId; });
            if (Game)
            {
                FMatchSnapshot Snap;
                FString SnapError;
                if (FLeagueService::BuildSnapshot(PostLeague, *Game, Snap, SnapError))
                {
                    LastRoundSnapshots.Add(MoveTemp(Snap));
                }
            }
        }
        RefreshDashboard();
        const FString PhaseLabel = LeagueSubsystem->GetSeasonPhase() == ESeasonPhase::Playoffs
            ? TEXT("Playoff") : TEXT("Round");
        StatusText->SetText(FText::FromString(FString::Printf(
            TEXT("%s complete  •  %d games resolved"), *PhaseLabel, Results.Num())));
        StatusText->SetColorAndOpacity(FSlateColor(DashboardStyle::Success));
        SetScreen(8);
    }
    else
    {
        StatusText->SetText(FText::FromString(Error));
        StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.32f, 0.28f, 1.0f)));
    }
}

void UManagementDashboardWidget::RefreshAwardsScreen(const FLeagueState& League, const FTeamState& Club)
{
    AwardsList->ClearChildren();
    if (League.Awards.Num() == 0)
    {
        AwardsList->AddChildToVerticalBox(MakeText(
            TEXT("Awards are announced at the end of the season."), 12, DashboardStyle::Secondary));
        return;
    }

    AwardsList->AddChildToVerticalBox(MakeText(
        FString::Printf(TEXT("SEASON %d AWARDS"), League.SeasonNumber), 14, DashboardStyle::Accent, true))
        ->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));

    auto AwardLabel = [](EAwardType Type) -> FString
    {
        switch (Type)
        {
        case EAwardType::MVP: return TEXT("MOST VALUABLE PLAYER");
        case EAwardType::DPOY: return TEXT("DEFENSIVE PLAYER OF THE YEAR");
        case EAwardType::ROY: return TEXT("ROOKIE OF THE YEAR");
        case EAwardType::MIP: return TEXT("MOST IMPROVED PLAYER");
        case EAwardType::ChampionMVP: return TEXT("CHAMPION MVP");
        default: return TEXT("AWARD");
        }
    };

    for (const FSeasonAward& Award : League.Awards)
    {
        const FPlayerProfile* Player = nullptr;
        const FTeamState* AwardTeam = nullptr;
        for (const FTeamState& Team : League.Teams)
        {
            Player = Team.Players.FindByPredicate(
                [&Award](const FPlayerProfile& P) { return P.PlayerId == Award.PlayerId; });
            if (Player) { AwardTeam = &Team; break; }
        }

        UBorder* Card = MakeCard(Award.TeamId == Club.TeamId
            ? DashboardStyle::AccentSoft : DashboardStyle::CardRaised);
        Card->SetPadding(FMargin(14.0f, 10.0f));
        UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
        Card->SetContent(Content);
        Content->AddChildToVerticalBox(MakeText(AwardLabel(Award.Type), 9, DashboardStyle::Accent, true));
        Content->AddChildToVerticalBox(MakeText(
            Player ? Player->DisplayName : TEXT("Unknown"), 16, DashboardStyle::Primary, true))
            ->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));

        FString Details;
        if (AwardTeam) { Details = AwardTeam->City + TEXT(" ") + AwardTeam->Nickname; }
        const FSeasonStats* Stats = League.SeasonStats.FindByPredicate(
            [&Award](const FSeasonStats& S) { return S.PlayerId == Award.PlayerId; });
        if (Stats && Stats->GamesPlayed > 0)
        {
            Details += FString::Printf(TEXT("  •  %.1f PPG  %.1f RPG  %.1f APG"),
                Stats->PPG(), Stats->RPG(), Stats->APG());
        }
        Content->AddChildToVerticalBox(MakeText(Details, 10, DashboardStyle::Secondary))
            ->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 0.0f));

        AwardsList->AddChildToVerticalBox(Card)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
    }
}

void UManagementDashboardWidget::RefreshOffseasonScreen(const FLeagueState& League, const FTeamState& Club)
{
    OffseasonList->ClearChildren();
    if (League.Phase != ESeasonPhase::Complete && League.Phase != ESeasonPhase::RegularSeason)
    {
        OffseasonList->AddChildToVerticalBox(MakeText(
            TEXT("The offseason begins after the playoffs."), 12, DashboardStyle::Secondary));
        return;
    }

    if (League.Phase == ESeasonPhase::RegularSeason && League.Offseason.CurrentStep == EOffseasonStep::Awards)
    {
        OffseasonList->AddChildToVerticalBox(MakeText(
            FString::Printf(TEXT("SEASON %d IN PROGRESS"), League.SeasonNumber),
            14, DashboardStyle::Accent, true));
        OffseasonList->AddChildToVerticalBox(MakeText(
            TEXT("The offseason will begin once the playoffs conclude."),
            11, DashboardStyle::Secondary))->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));
        return;
    }

    auto StepLabel = [](EOffseasonStep Step) -> FString
    {
        switch (Step)
        {
        case EOffseasonStep::Awards: return TEXT("AWARDS");
        case EOffseasonStep::Aging: return TEXT("AGING & DEVELOPMENT");
        case EOffseasonStep::ContractExpiry: return TEXT("CONTRACT EXPIRY");
        case EOffseasonStep::Draft: return TEXT("ROOKIE DRAFT");
        case EOffseasonStep::Resigning: return TEXT("RE-SIGNING");
        case EOffseasonStep::Complete: return TEXT("OFFSEASON COMPLETE");
        default: return TEXT("UNKNOWN");
        }
    };

    const FOffseasonState& Offseason = League.Offseason;
    OffseasonList->AddChildToVerticalBox(MakeText(
        FString::Printf(TEXT("CURRENT STEP: %s"), *StepLabel(Offseason.CurrentStep)),
        14, DashboardStyle::Accent, true))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));

    if (Offseason.CurrentStep == EOffseasonStep::Draft && Offseason.DraftClass.Num() > 0)
    {
        OffseasonList->AddChildToVerticalBox(MakeText(
            FString::Printf(TEXT("DRAFT CLASS  •  PICK %d"), Offseason.CurrentDraftPick + 1),
            11, DashboardStyle::Primary, true))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

        for (int32 Index = 0; Index < Offseason.DraftClass.Num(); ++Index)
        {
            const FDraftProspect& Prospect = Offseason.DraftClass[Index];
            const FLinearColor RowColor = Prospect.bDrafted ? DashboardStyle::Card : DashboardStyle::CardRaised;
            UBorder* Row = MakeCard(RowColor);
            Row->SetPadding(FMargin(12.0f, 6.0f));
            UHorizontalBox* RowContent = WidgetTree->ConstructWidget<UHorizontalBox>();
            Row->SetContent(RowContent);
            RowContent->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("#%d"), Index + 1), 10,
                DashboardStyle::Accent, true))->SetSize(DashboardStyle::Size(0.15f));
            RowContent->AddChildToHorizontalBox(MakeText(Prospect.Profile.DisplayName, 11,
                Prospect.bDrafted ? DashboardStyle::Secondary : DashboardStyle::Primary, !Prospect.bDrafted))
                ->SetSize(DashboardStyle::Size(1.0f));
            RowContent->AddChildToHorizontalBox(MakeText(DashboardStyle::Position(Prospect.Profile.Position), 10,
                DashboardStyle::Accent, true))->SetSize(DashboardStyle::Size(0.2f));
            RowContent->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("OVR %d"), Prospect.Profile.Ratings.Overall()),
                10, DashboardStyle::Primary, true))->SetSize(DashboardStyle::Size(0.3f));
            RowContent->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("AGE %d"), Prospect.Profile.Age),
                10, DashboardStyle::Secondary))->SetSize(DashboardStyle::Size(0.25f));
            if (Prospect.bDrafted)
            {
                const FTeamState* DraftedBy = League.Teams.FindByPredicate(
                    [&Prospect](const FTeamState& T) { return T.TeamId == Prospect.DraftedByTeamId; });
                UTextBlock* DraftedText = MakeText(
                    DraftedBy ? DraftedBy->Nickname : TEXT("???"), 9, DashboardStyle::Secondary);
                DraftedText->SetJustification(ETextJustify::Right);
                RowContent->AddChildToHorizontalBox(DraftedText)->SetSize(DashboardStyle::Size(0.4f));
            }
            else
            {
                UTextBlock* AvailText = MakeText(TEXT("AVAILABLE"), 9, DashboardStyle::Success, true);
                AvailText->SetJustification(ETextJustify::Right);
                RowContent->AddChildToHorizontalBox(AvailText)->SetSize(DashboardStyle::Size(0.4f));
            }
            OffseasonList->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 3.0f));
        }
    }

    if (Offseason.CurrentStep == EOffseasonStep::ContractExpiry && Offseason.ExpiredContractPlayerIds.Num() > 0)
    {
        OffseasonList->AddChildToVerticalBox(MakeText(
            FString::Printf(TEXT("%d CONTRACTS EXPIRED"), Offseason.ExpiredContractPlayerIds.Num()),
            11, DashboardStyle::Primary, true))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
    }

    if (Offseason.CurrentStep == EOffseasonStep::Complete)
    {
        OffseasonList->AddChildToVerticalBox(MakeText(
            TEXT("All offseason business is complete. Advance to begin the new season."),
            11, DashboardStyle::Success))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
    }

    UButton* AdvanceButton = WidgetTree->ConstructWidget<UButton>();
    AdvanceButton->SetBackgroundColor(DashboardStyle::Accent);
    const FString ButtonLabel = Offseason.CurrentStep == EOffseasonStep::Complete
        ? TEXT("START NEW SEASON") : FString::Printf(TEXT("ADVANCE: %s  >"), *StepLabel(Offseason.CurrentStep));
    AdvanceButton->SetContent(MakeText(ButtonLabel, 11, DashboardStyle::Background, true));
    AdvanceButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleAdvanceOffseason);
    OffseasonList->AddChildToVerticalBox(AdvanceButton)->SetPadding(FMargin(0.0f, 14.0f, 0.0f, 0.0f));
}

void UManagementDashboardWidget::HandleAdvanceOffseason()
{
    ULeagueGameSubsystem* Subsystem = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>();
    if (!Subsystem) { return; }
    FString Error;
    if (Subsystem->AdvanceOffseason(Error))
    {
        RefreshDashboard();
        if (Subsystem->GetSeasonPhase() == ESeasonPhase::RegularSeason)
        {
            StatusText->SetText(FText::FromString(FString::Printf(
                TEXT("Season %d has begun!"), Subsystem->GetSeasonNumber())));
            StatusText->SetColorAndOpacity(FSlateColor(DashboardStyle::Success));
            SimulateButtonText->SetText(FText::FromString(TEXT("SIMULATE ROUND  >")));
            SimulateButton->SetIsEnabled(true);
            SetScreen(0);
        }
        else
        {
            SetScreen(10);
        }
    }
    else
    {
        StatusText->SetText(FText::FromString(Error));
        StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.32f, 0.28f, 1.0f)));
    }
}
