#include "ManagementDashboardWidget.h"

#include "LeagueGameSubsystem.h"
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
        TEXT("STANDINGS"), TEXT("SCOUTING"), TEXT("TRAINING") };
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
    SimulateButton->SetContent(MakeText(TEXT("SIMULATE ROUND  >"), 14, DashboardStyle::Background, true));
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
    SeasonText->SetText(FText::FromString(FString::Printf(
        TEXT("REGULAR SEASON  •  ROUND %d OF 22"), FMath::Min(League.CurrentRound + 1, 22))));
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
    TArray<FMatchResult> Results;
    FString Error;
    if (LeagueSubsystem->AdvanceCurrentRound(Results, Error))
    {
        RefreshDashboard();
        StatusText->SetText(FText::FromString(FString::Printf(
            TEXT("Round complete  •  %d league games resolved"), Results.Num())));
        StatusText->SetColorAndOpacity(FSlateColor(DashboardStyle::Success));
    }
    else
    {
        StatusText->SetText(FText::FromString(Error));
        StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.32f, 0.28f, 1.0f)));
    }
}
