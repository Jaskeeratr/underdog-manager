#include "ManagementDashboardWidget.h"

#include "CommentaryService.h"
#include "ContractService.h"
#include "GameRecapService.h"
#include "FranchiseService.h"
#include "HighlightDirectorService.h"
#include "LeagueGameSubsystem.h"
#include "LeagueHistoryService.h"
#include "MatchPresentationService.h"
#include "RivalryService.h"
#include "TradeService.h"
#include "LeagueService.h"
#include "UnderdogCoreTypes.h"
#include "UnderdogPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Styling/CoreStyle.h"

namespace DashboardStyle
{
    const FLinearColor Background(0.009f, 0.011f, 0.015f, 1.0f);
    const FLinearColor Sidebar(0.018f, 0.021f, 0.027f, 1.0f);
    const FLinearColor Card(0.031f, 0.035f, 0.043f, 1.0f);
    const FLinearColor CardRaised(0.052f, 0.057f, 0.068f, 1.0f);
    const FLinearColor Stroke(0.105f, 0.115f, 0.135f, 1.0f);
    const FLinearColor Accent(0.94f, 0.10f, 0.16f, 1.0f);
    const FLinearColor AccentSoft(0.26f, 0.025f, 0.045f, 1.0f);
    const FLinearColor Primary(0.97f, 0.975f, 0.985f, 1.0f);
    const FLinearColor Secondary(0.58f, 0.61f, 0.67f, 1.0f);
    const FLinearColor Success(0.10f, 0.82f, 0.68f, 1.0f);
    const FLinearColor Gold(0.96f, 0.72f, 0.22f, 1.0f);

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
    const int32 AccessibleSize = Size <= 11 ? Size + 1 : Size;
    Widget->SetFont(FCoreStyle::GetDefaultFontStyle(
        bBold ? TEXT("Bold") : TEXT("Regular"), AccessibleSize));
    return Widget;
}

UBorder* UManagementDashboardWidget::MakeCard(const FLinearColor& Color)
{
    UBorder* CardWidget = WidgetTree->ConstructWidget<UBorder>();
    CardWidget->SetBrushColor(Color);
    CardWidget->SetPadding(FMargin(16.0f));
    return CardWidget;
}

UButton* UManagementDashboardWidget::MakeNavigationButton(const FString& Label, bool bActive)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>();
    Button->SetBackgroundColor(bActive ? DashboardStyle::AccentSoft : DashboardStyle::Sidebar);
    UTextBlock* LabelText = MakeText(Label, 10,
        bActive ? DashboardStyle::Accent : DashboardStyle::Secondary, bActive);
    LabelText->SetJustification(ETextJustify::Left);
    Button->SetContent(LabelText);
    Button->SetToolTipText(FText::FromString(FString::Printf(TEXT("Open %s"), *Label)));
    NavigationButtons.Add(Button);
    NavigationButtonLabels.Add(LabelText);
    return Button;
}

void UManagementDashboardWidget::BuildLayout()
{
    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
    WidgetTree->RootWidget = Root;

    UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
    Background->SetBrushColor(DashboardStyle::Background);
    Root->AddChildToOverlay(Background);

    USizeBox* AccentRail = WidgetTree->ConstructWidget<USizeBox>();
    AccentRail->SetHeightOverride(5.0f);
    UBorder* AccentFill = WidgetTree->ConstructWidget<UBorder>();
    AccentFill->SetBrushColor(DashboardStyle::Accent);
    AccentRail->SetContent(AccentFill);
    UOverlaySlot* AccentSlot = Root->AddChildToOverlay(AccentRail);
    AccentSlot->SetHorizontalAlignment(HAlign_Fill);
    AccentSlot->SetVerticalAlignment(VAlign_Top);

    UHorizontalBox* Shell = WidgetTree->ConstructWidget<UHorizontalBox>();
    UOverlaySlot* ShellOverlaySlot = Root->AddChildToOverlay(Shell);
    ShellOverlaySlot->SetHorizontalAlignment(HAlign_Fill);
    ShellOverlaySlot->SetVerticalAlignment(VAlign_Fill);

    UBorder* Sidebar = WidgetTree->ConstructWidget<UBorder>();
    Sidebar->SetBrushColor(DashboardStyle::Sidebar);
    Sidebar->SetPadding(FMargin(20.0f, 28.0f));
    UHorizontalBoxSlot* SidebarSlot = Shell->AddChildToHorizontalBox(Sidebar);
    SidebarSlot->SetSize(DashboardStyle::Size(0.17f));

    UScrollBox* NavigationScroll = WidgetTree->ConstructWidget<UScrollBox>();
    NavigationScroll->SetScrollBarVisibility(ESlateVisibility::Visible);
    Sidebar->SetContent(NavigationScroll);
    UVerticalBox* NavigationBox = WidgetTree->ConstructWidget<UVerticalBox>();
    NavigationScroll->AddChild(NavigationBox);
    UHorizontalBox* Brand = WidgetTree->ConstructWidget<UHorizontalBox>();
    NavigationBox->AddChildToVerticalBox(Brand)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));
    USizeBox* BrandMarkSize = WidgetTree->ConstructWidget<USizeBox>();
    BrandMarkSize->SetWidthOverride(42.0f);
    BrandMarkSize->SetHeightOverride(42.0f);
    UBorder* BrandMark = MakeCard(DashboardStyle::Accent);
    BrandMark->SetPadding(FMargin(0.0f));
    UTextBlock* BrandLetter = MakeText(TEXT("U"), 22, DashboardStyle::Primary, true);
    BrandLetter->SetJustification(ETextJustify::Center);
    BrandMark->SetContent(BrandLetter);
    BrandMarkSize->SetContent(BrandMark);
    Brand->AddChildToHorizontalBox(BrandMarkSize)->SetVerticalAlignment(VAlign_Center);
    UVerticalBox* BrandCopy = WidgetTree->ConstructWidget<UVerticalBox>();
    Brand->AddChildToHorizontalBox(BrandCopy)->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));
    BrandCopy->AddChildToVerticalBox(MakeText(TEXT("UNDERDOG"), 20, DashboardStyle::Primary, true));
    BrandCopy->AddChildToVerticalBox(MakeText(TEXT("MANAGER"), 9, DashboardStyle::Accent, true));

    const FString Labels[] = { TEXT("OVERVIEW"), TEXT("ROSTER"), TEXT("SCHEDULE"),
        TEXT("STANDINGS"), TEXT("SCOUTING"), TEXT("TRAINING"), TEXT("TACTICS"),
        TEXT("TRADES"), TEXT("PLAYOFFS"), TEXT("AWARDS"), TEXT("OFFSEASON"), TEXT("SAVE / LOAD"),
        TEXT("GAME RECAP"), TEXT("HISTORY"), TEXT("CONTRACTS"), TEXT("RIVALRIES"), TEXT("FRONT OFFICE"),
        TEXT("STAFF & COACHING"), TEXT("MANAGER CAREER") };
    const int32 ScreenIndices[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15, 16, 19, 20, 21 };
    NavigationButtons.Reset();
    NavigationButtonLabels.Reset();
    NavigationScreenIndices.Reset();
    NavigationNames.Reset();
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Labels); ++Index)
    {
        if (Index == 0 || Index == 4 || Index == 8 || Index == 13 || Index == 16)
        {
            const TCHAR* Section = Index == 0 ? TEXT("CLUB") : Index == 4 ? TEXT("BASKETBALL")
                : Index == 8 ? TEXT("SEASON") : Index == 13 ? TEXT("LEAGUE") : TEXT("ORGANIZATION");
            NavigationBox->AddChildToVerticalBox(MakeText(Section, 8, DashboardStyle::Secondary, true))
                ->SetPadding(FMargin(4.0f, Index == 0 ? 0.0f : 12.0f, 0.0f, 6.0f));
        }
        UButton* NavigationButton = MakeNavigationButton(Labels[Index], Index == 0);
        NavigationScreenIndices.Add(ScreenIndices[Index]);
        NavigationNames.Add(Labels[Index]);
        switch (Index)
        {
        case 0: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowOverview); break;
        case 1: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowRoster); break;
        case 2: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowSchedule); break;
        case 3: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowStandings); break;
        case 4: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowScouting); break;
        case 5: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowTraining); break;
        case 6: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowTactics); break;
        case 7: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowTrades); break;
        case 8: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowPlayoffs); break;
        case 9: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowAwards); break;
        case 10: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowOffseason); break;
        case 11: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowSaveLoad); break;
        case 12: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowGameRecap); break;
        case 13: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowHistory); break;
        case 14: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowContracts); break;
        case 15: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowRivalries); break;
        case 16: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowFranchise); break;
        case 17: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowStaff); break;
        case 18: NavigationButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowCareer); break;
        default: break;
        }
        NavigationBox->AddChildToVerticalBox(NavigationButton)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
    }

    UVerticalBox* ContentColumn = WidgetTree->ConstructWidget<UVerticalBox>();
    UHorizontalBoxSlot* ContentSlot = Shell->AddChildToHorizontalBox(ContentColumn);
    ContentSlot->SetSize(DashboardStyle::Size(0.83f));

    UBorder* UtilityBar = MakeCard(DashboardStyle::Sidebar);
    UtilityBar->SetPadding(FMargin(30.0f, 14.0f, 34.0f, 14.0f));
    UHorizontalBox* UtilityContent = WidgetTree->ConstructWidget<UHorizontalBox>();
    UtilityBar->SetContent(UtilityContent);
    CurrentScreenText = MakeText(TEXT("COMMAND CENTER  /  OVERVIEW"), 9, DashboardStyle::Secondary, true);
    UtilityContent->AddChildToHorizontalBox(CurrentScreenText)->SetSize(DashboardStyle::Size(1.0f));
    GlobalSeasonText = MakeText(TEXT("SEASON 1"), 9, DashboardStyle::Gold, true);
    UtilityContent->AddChildToHorizontalBox(GlobalSeasonText)->SetSize(
        DashboardStyle::Size(1.0f, ESlateSizeRule::Automatic));
    ContentColumn->AddChildToVerticalBox(UtilityBar);

    ScreenSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>();
    UVerticalBoxSlot* SwitcherSlot = ContentColumn->AddChildToVerticalBox(ScreenSwitcher);
    SwitcherSlot->SetSize(DashboardStyle::Size(1.0f));
    SwitcherSlot->SetPadding(FMargin(30.0f, 24.0f, 34.0f, 24.0f));

    UVerticalBox* Main = WidgetTree->ConstructWidget<UVerticalBox>();
    ScreenSwitcher->AddChild(Main);

    UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
    Main->AddChildToVerticalBox(Header)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
    USizeBox* ClubMarkSize = WidgetTree->ConstructWidget<USizeBox>();
    ClubMarkSize->SetWidthOverride(62.0f);
    ClubMarkSize->SetHeightOverride(62.0f);
    UBorder* ClubMark = MakeCard(DashboardStyle::AccentSoft);
    ClubMark->SetPadding(FMargin(0.0f));
    UTextBlock* ClubMonogram = MakeText(TEXT("UC"), 18, DashboardStyle::Accent, true);
    ClubMonogram->SetJustification(ETextJustify::Center);
    ClubMark->SetContent(ClubMonogram);
    ClubMarkSize->SetContent(ClubMark);
    Header->AddChildToHorizontalBox(ClubMarkSize)->SetVerticalAlignment(VAlign_Center);
    UVerticalBox* HeaderCopy = WidgetTree->ConstructWidget<UVerticalBox>();
    UHorizontalBoxSlot* HeaderCopySlot = Header->AddChildToHorizontalBox(HeaderCopy);
    HeaderCopySlot->SetSize(DashboardStyle::Size(1.0f));
    HeaderCopySlot->SetPadding(FMargin(16.0f, 0.0f, 0.0f, 0.0f));
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
        UHorizontalBoxSlot* MetricSlot = Metrics->AddChildToHorizontalBox(CardWidget);
        MetricSlot->SetSize(DashboardStyle::Size(1.0f));
        MetricSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));
    };
    AddMetric(TEXT("SEASON RECORD"), RecordText, DashboardStyle::Primary);
    AddMetric(TEXT("CHEMISTRY"), ChemistryText, DashboardStyle::Accent);
    AddMetric(TEXT("SALARY / CAP"), SalaryCapText, DashboardStyle::Success);

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
    ScreenSwitcher->AddChild(MakeDetailScreen(TEXT("GAME PLAN"), TEXT("Tactics"),
        TEXT("Set pace, offense, defense, and rebounding strategies. Tactics affect simulation outcomes."), TacticsList));
    ScreenSwitcher->AddChild(MakeDetailScreen(TEXT("TRANSACTIONS"), TEXT("Trade Centre"),
        TEXT("Propose trades with other teams. The trade deadline is round 16."), TradeList));
    ScreenSwitcher->AddChild(MakeDetailScreen(TEXT("POSTSEASON"), TEXT("Playoff Bracket"),
        TEXT("The top four teams compete in best-of-three semifinals and finals."), PlayoffList));
    ScreenSwitcher->AddChild(MakeDetailScreen(TEXT("RESULTS"), TEXT("Round Results"),
        TEXT("Box scores and outcomes from the most recent round of games."), ResultsList));
    ScreenSwitcher->AddChild(MakeDetailScreen(TEXT("RECOGNITION"), TEXT("Season Awards"),
        TEXT("MVP, Defensive Player of the Year, Rookie of the Year, and Most Improved."), AwardsList));
    ScreenSwitcher->AddChild(MakeDetailScreen(TEXT("FRONT OFFICE"), TEXT("Offseason"),
        TEXT("Step through aging, contracts, the draft, and re-signing to begin a new season."), OffseasonList));
    ScreenSwitcher->AddChild(MakeDetailScreen(TEXT("DATA MANAGEMENT"), TEXT("Save / Load"),
        TEXT("Save your progress or load a previous save. Three slots available."), SaveLoadList));
    ScreenSwitcher->AddChild(MakeDetailScreen(TEXT("GAME DAY"), TEXT("Game Recap"),
        TEXT("Quarter-by-quarter breakdown and highlight plays from the latest round."), GameRecapList));
    ScreenSwitcher->AddChild(MakeDetailScreen(TEXT("LEGACY"), TEXT("League History"),
        TEXT("Championship banners, all-time stat leaders, and hall of fame."), HistoryList));
    ScreenSwitcher->AddChild(MakeDetailScreen(TEXT("FRONT OFFICE"), TEXT("Contract Extensions"),
        TEXT("Negotiate extensions with players approaching free agency."), ContractsList));
    ScreenSwitcher->AddChild(MakeDetailScreen(TEXT("COMPETITION"), TEXT("Rivalries"),
        TEXT("Teams that have clashed in close games and playoff battles."), RivalriesList));
    ScreenSwitcher->AddChild(MakeDetailScreen(TEXT("MATCH DAY"), TEXT("Match Center"),
        TEXT("Pre-game analysis. Choose to watch highlights or get the instant result."), MatchCenterList));
    ScreenSwitcher->AddChild(MakeDetailScreen(TEXT("POST-GAME"), TEXT("Recap"),
        TEXT("Full box score, highlight reel, and quarter-by-quarter breakdown."), PostGameList));
    ScreenSwitcher->AddChild(MakeDetailScreen(TEXT("FRANCHISE OPERATIONS"), TEXT("Front Office"),
        TEXT("Control ticket pricing, facilities, fan support, finances, and owner expectations."), FranchiseList));
    ScreenSwitcher->AddChild(MakeDetailScreen(TEXT("BASKETBALL OPERATIONS"), TEXT("Staff & Coaching"),
        TEXT("Build a coaching team whose strengths shape tactics, development, scouting, and player health."), StaffList));
    ScreenSwitcher->AddChild(MakeDetailScreen(TEXT("LEGACY"), TEXT("Manager Career"),
        TEXT("Track your record, job security, championships, club history, and available opportunities."), CareerList));
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
    UScrollBox* ContentScroll = WidgetTree->ConstructWidget<UScrollBox>();
    ContentScroll->SetScrollBarVisibility(ESlateVisibility::Visible);
    ContentScroll->AddChild(OutList);
    ContentCard->SetContent(ContentScroll);
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
    if (GlobalSeasonText)
    {
        GlobalSeasonText->SetText(FText::FromString(FString::Printf(
            TEXT("SEASON %d   /   MANAGER REP %d"),
            League.SeasonNumber, League.ManagerCareer.CareerScore)));
    }
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
    ChemistryText->SetText(FText::FromString(FString::Printf(TEXT("%d"), Club.Chemistry)));
    ChemistryText->SetColorAndOpacity(FSlateColor(Club.Chemistry >= 65 ? DashboardStyle::Success
        : Club.Chemistry <= 35 ? FLinearColor(0.95f, 0.32f, 0.28f, 1.0f) : DashboardStyle::Accent));
    SalaryCapText->SetText(FText::FromString(FString::Printf(
        TEXT("$%.1fM / $%.1fM"), Club.TotalSalary() / 100000000.0, Club.SalaryCapMinorUnits / 100000000.0)));
    SalaryCapText->SetColorAndOpacity(FSlateColor(Club.IsOverCap()
        ? FLinearColor(0.95f, 0.32f, 0.28f, 1.0f) : DashboardStyle::Success));

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
    RefreshTacticsScreen(Club);
    RefreshTradeScreen(League, Club);
    RefreshPlayoffScreen(League, Club);
    RefreshResultsScreen(LastRoundResults, League, Club);
    RefreshAwardsScreen(League, Club);
    RefreshOffseasonScreen(League, Club);
    RefreshSaveLoadScreen();
    RefreshGameRecapScreen(League, Club);
    RefreshHistoryScreen(League);
    RefreshContractsScreen(League, Club);
    RefreshRivalriesScreen(League, Club);
    RefreshMatchCenterScreen(League, Club);
    RefreshPostGameScreen();
    RefreshFranchiseScreen(League, Club);
    RefreshStaffScreen(League, Club);
    RefreshCareerScreen(League);
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
        if (State->InjuryGamesRemaining > 0)
        {
            Row->AddChildToHorizontalBox(MakeText(
                FString::Printf(TEXT("INJ %d"), State->InjuryGamesRemaining), 10,
                FLinearColor(0.95f, 0.32f, 0.28f, 1.0f), true))
                ->SetSize(DashboardStyle::Size(0.3f));
            UTextBlock* InjDesc = MakeText(
                State->InjuryDescription.IsEmpty() ? TEXT("Injured") : State->InjuryDescription,
                9, FLinearColor(0.95f, 0.32f, 0.28f, 1.0f));
            InjDesc->SetJustification(ETextJustify::Right);
            Row->AddChildToHorizontalBox(InjDesc)->SetSize(DashboardStyle::Size(0.87f));
        }
        else
        {
            Row->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("FIT %d"), State->Fitness), 10,
                State->Fitness >= 70 ? DashboardStyle::Success : DashboardStyle::Accent, true))
                ->SetSize(DashboardStyle::Size(0.3f));
            Row->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("MOR %d"), State->Morale), 10,
                State->Morale >= 60 ? DashboardStyle::Success
                : State->Morale <= 35 ? FLinearColor(0.95f, 0.32f, 0.28f, 1.0f) : DashboardStyle::Secondary, true))
                ->SetSize(DashboardStyle::Size(0.3f));
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
        }
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

void UManagementDashboardWidget::RefreshTacticsScreen(const FTeamState& Club)
{
    TacticsList->ClearChildren();

    auto PaceLabel = [](EPaceStyle S) { return S == EPaceStyle::Fast ? TEXT("FAST") : S == EPaceStyle::Slow ? TEXT("SLOW") : TEXT("BALANCED"); };
    auto OffenseLabel = [](EOffenseStyle S) { return S == EOffenseStyle::Perimeter ? TEXT("PERIMETER") : S == EOffenseStyle::Inside ? TEXT("INSIDE") : TEXT("BALANCED"); };
    auto DefenseLabel = [](EDefenseStyle S) { return S == EDefenseStyle::Zone ? TEXT("ZONE") : S == EDefenseStyle::Switching ? TEXT("SWITCHING") : TEXT("MAN-TO-MAN"); };
    auto ReboundLabel = [](EReboundPriority S) { return S == EReboundPriority::CrashBoards ? TEXT("CRASH BOARDS") : S == EReboundPriority::Transition ? TEXT("TRANSITION") : TEXT("BALANCED"); };

    TacticsList->AddChildToVerticalBox(MakeText(FString::Printf(TEXT("CURRENT TACTICS  •  %s PACE  •  %s OFFENSE  •  %s DEFENSE  •  %s REBOUNDING"),
        PaceLabel(Club.Tactics.Pace), OffenseLabel(Club.Tactics.Offense),
        DefenseLabel(Club.Tactics.Defense), ReboundLabel(Club.Tactics.Rebounding)),
        11, DashboardStyle::Accent, true))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));

    auto AddTacticButton = [this](const FString& Label, const FName HandlerName, bool bActive)
    {
        UButton* Button = WidgetTree->ConstructWidget<UButton>();
        Button->SetBackgroundColor(bActive ? DashboardStyle::AccentSoft : DashboardStyle::CardRaised);
        Button->SetContent(MakeText(Label, 11, bActive ? DashboardStyle::Accent : DashboardStyle::Primary, bActive));
        FScriptDelegate Handler;
        Handler.BindUFunction(this, HandlerName);
        Button->OnClicked.Add(Handler);
        TacticsList->AddChildToVerticalBox(Button)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
    };

    TacticsList->AddChildToVerticalBox(MakeText(TEXT("PACE"), 10, DashboardStyle::Primary, true))
        ->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
    AddTacticButton(TEXT("SLOW  •  LONGER POSSESSIONS, DELIBERATE PLAY"), GET_FUNCTION_NAME_CHECKED(UManagementDashboardWidget, SetTacticsPaceSlow), Club.Tactics.Pace == EPaceStyle::Slow);
    AddTacticButton(TEXT("BALANCED  •  STANDARD TEMPO"), GET_FUNCTION_NAME_CHECKED(UManagementDashboardWidget, SetTacticsPaceBalanced), Club.Tactics.Pace == EPaceStyle::Balanced);
    AddTacticButton(TEXT("FAST  •  QUICK POSSESSIONS, UP-TEMPO"), GET_FUNCTION_NAME_CHECKED(UManagementDashboardWidget, SetTacticsPaceFast), Club.Tactics.Pace == EPaceStyle::Fast);

    TacticsList->AddChildToVerticalBox(MakeText(TEXT("OFFENSE"), 10, DashboardStyle::Primary, true))
        ->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 6.0f));
    AddTacticButton(TEXT("INSIDE  •  POST PLAY AND DRIVES"), GET_FUNCTION_NAME_CHECKED(UManagementDashboardWidget, SetTacticsOffenseInside), Club.Tactics.Offense == EOffenseStyle::Inside);
    AddTacticButton(TEXT("BALANCED  •  MIXED ATTACK"), GET_FUNCTION_NAME_CHECKED(UManagementDashboardWidget, SetTacticsOffenseBalanced), Club.Tactics.Offense == EOffenseStyle::Balanced);
    AddTacticButton(TEXT("PERIMETER  •  THREE-POINT SHOOTING"), GET_FUNCTION_NAME_CHECKED(UManagementDashboardWidget, SetTacticsOffensePerimeter), Club.Tactics.Offense == EOffenseStyle::Perimeter);

    TacticsList->AddChildToVerticalBox(MakeText(TEXT("DEFENSE"), 10, DashboardStyle::Primary, true))
        ->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 6.0f));
    AddTacticButton(TEXT("MAN-TO-MAN  •  STANDARD INDIVIDUAL MATCHUPS"), GET_FUNCTION_NAME_CHECKED(UManagementDashboardWidget, SetTacticsDefenseMan), Club.Tactics.Defense == EDefenseStyle::Man);
    AddTacticButton(TEXT("SWITCHING  •  FLEXIBLE MATCHUPS, +TURNOVERS"), GET_FUNCTION_NAME_CHECKED(UManagementDashboardWidget, SetTacticsDefenseSwitching), Club.Tactics.Defense == EDefenseStyle::Switching);
    AddTacticButton(TEXT("ZONE  •  PAINT PROTECTION, +BLOCKS +TURNOVERS"), GET_FUNCTION_NAME_CHECKED(UManagementDashboardWidget, SetTacticsDefenseZone), Club.Tactics.Defense == EDefenseStyle::Zone);

    TacticsList->AddChildToVerticalBox(MakeText(TEXT("REBOUNDING"), 10, DashboardStyle::Primary, true))
        ->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 6.0f));
    AddTacticButton(TEXT("TRANSITION  •  FAST BREAK PRIORITY"), GET_FUNCTION_NAME_CHECKED(UManagementDashboardWidget, SetTacticsReboundTransition), Club.Tactics.Rebounding == EReboundPriority::Transition);
    AddTacticButton(TEXT("BALANCED  •  STANDARD BOARD WORK"), GET_FUNCTION_NAME_CHECKED(UManagementDashboardWidget, SetTacticsReboundBalanced), Club.Tactics.Rebounding == EReboundPriority::Balanced);
    AddTacticButton(TEXT("CRASH BOARDS  •  OFFENSIVE REBOUNDS, +POSSESSIONS"), GET_FUNCTION_NAME_CHECKED(UManagementDashboardWidget, SetTacticsReboundCrash), Club.Tactics.Rebounding == EReboundPriority::CrashBoards);
}

void UManagementDashboardWidget::SetScreen(int32 Index)
{
    if (ScreenSwitcher) { ScreenSwitcher->SetActiveWidgetIndex(Index); }
    for (int32 NavIndex = 0; NavIndex < NavigationButtons.Num(); ++NavIndex)
    {
        const bool bActive = NavigationScreenIndices.IsValidIndex(NavIndex)
            && NavigationScreenIndices[NavIndex] == Index;
        if (NavigationButtons[NavIndex])
        {
            NavigationButtons[NavIndex]->SetBackgroundColor(
                bActive ? DashboardStyle::AccentSoft : DashboardStyle::Sidebar);
        }
        if (NavigationButtonLabels.IsValidIndex(NavIndex) && NavigationButtonLabels[NavIndex])
        {
            NavigationButtonLabels[NavIndex]->SetColorAndOpacity(FSlateColor(
                bActive ? DashboardStyle::Accent : DashboardStyle::Secondary));
            NavigationButtonLabels[NavIndex]->SetFont(FCoreStyle::GetDefaultFontStyle(
                bActive ? TEXT("Bold") : TEXT("Regular"), 11));
        }
        if (bActive && CurrentScreenText && NavigationNames.IsValidIndex(NavIndex))
        {
            CurrentScreenText->SetText(FText::FromString(FString::Printf(
                TEXT("COMMAND CENTER  /  %s"), *NavigationNames[NavIndex])));
        }
    }
}

void UManagementDashboardWidget::ShowOverview() { SetScreen(0); }
void UManagementDashboardWidget::ShowRoster() { SetScreen(1); }
void UManagementDashboardWidget::ShowSchedule() { SetScreen(2); }
void UManagementDashboardWidget::ShowStandings() { SetScreen(3); }
void UManagementDashboardWidget::ShowScouting() { SetScreen(4); }
void UManagementDashboardWidget::ShowTraining() { SetScreen(5); }
void UManagementDashboardWidget::ShowTactics() { SetScreen(6); }
void UManagementDashboardWidget::ShowTrades() { SetScreen(7); }
void UManagementDashboardWidget::ShowPlayoffs() { SetScreen(8); }
void UManagementDashboardWidget::ShowAwards() { SetScreen(10); }
void UManagementDashboardWidget::ShowOffseason() { SetScreen(11); }
void UManagementDashboardWidget::ShowSaveLoad() { SetScreen(12); }
void UManagementDashboardWidget::ShowGameRecap() { SetScreen(13); }
void UManagementDashboardWidget::ShowHistory() { SetScreen(14); }
void UManagementDashboardWidget::ShowContracts() { SetScreen(15); }
void UManagementDashboardWidget::ShowRivalries() { SetScreen(16); }
void UManagementDashboardWidget::ShowMatchCenter() { SetScreen(17); }
void UManagementDashboardWidget::ShowPostGame() { SetScreen(18); }
void UManagementDashboardWidget::ShowFranchise() { SetScreen(19); }
void UManagementDashboardWidget::ShowStaff() { SetScreen(20); }
void UManagementDashboardWidget::ShowCareer() { SetScreen(21); }

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

    const bool bOpen = League.Phase == ESeasonPhase::RegularSeason
        && League.CurrentRound < League.TradeDeadlineRound;
    TradeList->AddChildToVerticalBox(MakeText(
        bOpen ? FString::Printf(TEXT("TRADE WINDOW OPEN  •  DEADLINE ROUND %d"), League.TradeDeadlineRound)
              : TEXT("TRADE WINDOW CLOSED"),
        11, bOpen ? DashboardStyle::Success : DashboardStyle::Secondary, true))
        ->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));

    if (bOpen)
    {
        if (!SelectedTradeTeamId.IsValid())
        {
            TradeList->AddChildToVerticalBox(MakeText(TEXT("SELECT TRADE PARTNER"), 10,
                DashboardStyle::Accent, true))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
            DisplayedTeamIds.Reset();
            int32 Shown = 0;
            for (const FTeamState& Team : League.Teams)
            {
                if (Team.TeamId == Club.TeamId || Shown >= 8) { continue; }
                DisplayedTeamIds.Add(Team.TeamId);
                UButton* TeamBtn = WidgetTree->ConstructWidget<UButton>();
                TeamBtn->SetBackgroundColor(DashboardStyle::CardRaised);
                TeamBtn->SetContent(MakeText(FString::Printf(TEXT("%s %s  •  %d-%d  •  %d players"),
                    *Team.City, *Team.Nickname, Team.Wins, Team.Losses, Team.Players.Num()),
                    11, DashboardStyle::Primary, true));
                BindTeamButton(TeamBtn, Shown);
                TradeList->AddChildToVerticalBox(TeamBtn)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
                Shown++;
            }
        }
        else
        {
            const FTeamState* Partner = League.Teams.FindByPredicate(
                [this](const FTeamState& T) { return T.TeamId == SelectedTradeTeamId; });
            if (!Partner) { SelectedTradeTeamId.Invalidate(); return; }

            TradeList->AddChildToVerticalBox(MakeText(
                FString::Printf(TEXT("TRADING WITH: %s %s"), *Partner->City.ToUpper(), *Partner->Nickname.ToUpper()),
                11, DashboardStyle::Accent, true))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));

            UButton* ClearBtn = WidgetTree->ConstructWidget<UButton>();
            ClearBtn->SetBackgroundColor(DashboardStyle::Card);
            ClearBtn->SetContent(MakeText(TEXT("CHANGE PARTNER"), 9, DashboardStyle::Secondary, true));
            ClearBtn->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleClearTrade);
            TradeList->AddChildToVerticalBox(ClearBtn)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));

            TradeList->AddChildToVerticalBox(MakeText(TEXT("YOUR PLAYERS  •  CLICK TO OFFER"), 10,
                DashboardStyle::Accent, true))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
            DisplayedOutgoingIds.Reset();
            for (int32 Idx = 0; Idx < FMath::Min(8, Club.Players.Num()); ++Idx)
            {
                const FPlayerProfile& P = Club.Players[Idx];
                DisplayedOutgoingIds.Add(P.PlayerId);
                const bool bSelected = TradeOutgoingPlayerIds.Contains(P.PlayerId);
                UButton* Btn = WidgetTree->ConstructWidget<UButton>();
                Btn->SetBackgroundColor(bSelected ? DashboardStyle::AccentSoft : DashboardStyle::CardRaised);
                Btn->SetContent(MakeText(FString::Printf(TEXT("%s%s  •  %s  •  OVR %d"),
                    bSelected ? TEXT("[SEND] ") : TEXT(""),
                    *P.DisplayName, *DashboardStyle::Position(P.Position), P.Ratings.Overall()),
                    10, bSelected ? DashboardStyle::Accent : DashboardStyle::Primary, bSelected));
                BindOutgoingButton(Btn, Idx);
                TradeList->AddChildToVerticalBox(Btn)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 2.0f));
            }

            TradeList->AddChildToVerticalBox(MakeText(TEXT("THEIR PLAYERS  •  CLICK TO REQUEST"), 10,
                DashboardStyle::Accent, true))->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 6.0f));
            DisplayedIncomingIds.Reset();
            for (int32 Idx = 0; Idx < FMath::Min(8, Partner->Players.Num()); ++Idx)
            {
                const FPlayerProfile& P = Partner->Players[Idx];
                DisplayedIncomingIds.Add(P.PlayerId);
                const bool bSelected = TradeIncomingPlayerIds.Contains(P.PlayerId);
                UButton* Btn = WidgetTree->ConstructWidget<UButton>();
                Btn->SetBackgroundColor(bSelected ? DashboardStyle::AccentSoft : DashboardStyle::CardRaised);
                Btn->SetContent(MakeText(FString::Printf(TEXT("%s%s  •  %s  •  OVR %d"),
                    bSelected ? TEXT("[GET] ") : TEXT(""),
                    *P.DisplayName, *DashboardStyle::Position(P.Position), P.Ratings.Overall()),
                    10, bSelected ? DashboardStyle::Accent : DashboardStyle::Primary, bSelected));
                BindIncomingButton(Btn, Idx);
                TradeList->AddChildToVerticalBox(Btn)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 2.0f));
            }

            if (TradeOutgoingPlayerIds.Num() > 0 && TradeIncomingPlayerIds.Num() > 0)
            {
                FTradeOffer PreviewOffer;
                PreviewOffer.ProposingTeamId = Club.TeamId;
                PreviewOffer.ReceivingTeamId = Partner->TeamId;
                for (const FGuid& Id : TradeOutgoingPlayerIds)
                {
                    FTradeAsset Asset; Asset.PlayerId = Id; Asset.FromTeamId = Club.TeamId;
                    PreviewOffer.Outgoing.Add(Asset);
                }
                for (const FGuid& Id : TradeIncomingPlayerIds)
                {
                    FTradeAsset Asset; Asset.PlayerId = Id; Asset.FromTeamId = Partner->TeamId;
                    PreviewOffer.Incoming.Add(Asset);
                }
                const FTradeEvaluation Evaluation = FTradeService::EvaluateTradeDetailed(League, PreviewOffer);
                UBorder* EvaluationCard = MakeCard(Evaluation.bAccepted
                    ? FLinearColor(0.02f, 0.18f, 0.14f, 1.0f) : DashboardStyle::AccentSoft);
                EvaluationCard->SetPadding(FMargin(12.0f, 9.0f));
                UVerticalBox* EvaluationContent = WidgetTree->ConstructWidget<UVerticalBox>();
                EvaluationCard->SetContent(EvaluationContent);
                EvaluationContent->AddChildToVerticalBox(MakeText(Evaluation.Summary, 11,
                    Evaluation.bAccepted ? DashboardStyle::Success : DashboardStyle::Accent, true));
                EvaluationContent->AddChildToVerticalBox(MakeText(FString::Printf(
                    TEXT("YOU RECEIVE %d VALUE  |  THEY RECEIVE %d VALUE  |  SALARY CHANGE %+.1fM"),
                    Evaluation.ProposerReceivesValue, Evaluation.ReceiverReceivesValue,
                    Evaluation.ProposerSalaryChange / 100000000.0), 9, DashboardStyle::Secondary));
                for (const FString& Reason : Evaluation.Reasons)
                {
                    EvaluationContent->AddChildToVerticalBox(MakeText(TEXT("- ") + Reason, 9, DashboardStyle::Secondary));
                }
                TradeList->AddChildToVerticalBox(EvaluationCard)->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 5.0f));

                UButton* ProposeBtn = WidgetTree->ConstructWidget<UButton>();
                ProposeBtn->SetBackgroundColor(DashboardStyle::Accent);
                ProposeBtn->SetContent(MakeText(FString::Printf(TEXT("PROPOSE TRADE  •  %d FOR %d"),
                    TradeOutgoingPlayerIds.Num(), TradeIncomingPlayerIds.Num()),
                    11, DashboardStyle::Background, true));
                ProposeBtn->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleBuildTrade);
                ProposeBtn->SetIsEnabled(Evaluation.bLegal);
                TradeList->AddChildToVerticalBox(ProposeBtn)->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
            }
        }
    }

    TradeList->AddChildToVerticalBox(MakeText(TEXT("TRADE HISTORY"), 10,
        DashboardStyle::Accent, true))->SetPadding(FMargin(0.0f, 14.0f, 0.0f, 8.0f));
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

void UManagementDashboardWidget::HandleProposeTrade() {}

void UManagementDashboardWidget::HandleBuildTrade()
{
    ULeagueGameSubsystem* Subsystem = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>();
    const FLeagueState League = Subsystem->GetLeague();
    if (League.Teams.Num() == 0 || !SelectedTradeTeamId.IsValid()) { return; }
    if (TradeOutgoingPlayerIds.Num() == 0 || TradeIncomingPlayerIds.Num() == 0) { return; }
    FString Error;
    if (Subsystem->ProposeTrade(League.Teams[0].TeamId, TradeOutgoingPlayerIds,
        SelectedTradeTeamId, TradeIncomingPlayerIds, Error))
    {
        TradeOutgoingPlayerIds.Reset();
        TradeIncomingPlayerIds.Reset();
        SelectedTradeTeamId.Invalidate();
        RefreshDashboard();
        SetScreen(7);
        StatusText->SetText(FText::FromString(TEXT("Trade accepted and executed!")));
        StatusText->SetColorAndOpacity(FSlateColor(DashboardStyle::Success));
    }
    else
    {
        RefreshDashboard();
        SetScreen(7);
        StatusText->SetText(FText::FromString(Error));
        StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.32f, 0.28f, 1.0f)));
    }
}

void UManagementDashboardWidget::HandleClearTrade()
{
    SelectedTradeTeamId.Invalidate();
    TradeOutgoingPlayerIds.Reset();
    TradeIncomingPlayerIds.Reset();
    RefreshDashboard();
    SetScreen(7);
}

void UManagementDashboardWidget::BindTeamButton(UButton* Button, int32 Index)
{
    switch (Index) {
    case 0: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleTeam0); break;
    case 1: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleTeam1); break;
    case 2: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleTeam2); break;
    case 3: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleTeam3); break;
    case 4: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleTeam4); break;
    case 5: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleTeam5); break;
    case 6: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleTeam6); break;
    case 7: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleTeam7); break;
    default: break;
    }
}

void UManagementDashboardWidget::BindOutgoingButton(UButton* Button, int32 Index)
{
    switch (Index) {
    case 0: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleOut0); break;
    case 1: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleOut1); break;
    case 2: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleOut2); break;
    case 3: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleOut3); break;
    case 4: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleOut4); break;
    case 5: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleOut5); break;
    case 6: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleOut6); break;
    case 7: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleOut7); break;
    default: break;
    }
}

void UManagementDashboardWidget::BindIncomingButton(UButton* Button, int32 Index)
{
    switch (Index) {
    case 0: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleIn0); break;
    case 1: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleIn1); break;
    case 2: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleIn2); break;
    case 3: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleIn3); break;
    case 4: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleIn4); break;
    case 5: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleIn5); break;
    case 6: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleIn6); break;
    case 7: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleIn7); break;
    default: break;
    }
}

void UManagementDashboardWidget::BindDraftButton(UButton* Button, int32 Index)
{
    switch (Index) {
    case 0: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleDraft0); break;
    case 1: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleDraft1); break;
    case 2: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleDraft2); break;
    case 3: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleDraft3); break;
    case 4: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleDraft4); break;
    case 5: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleDraft5); break;
    case 6: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleDraft6); break;
    case 7: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleDraft7); break;
    default: break;
    }
}

void UManagementDashboardWidget::BindSaveButton(UButton* Button, int32 Index)
{
    if (Index == 0) { Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleSaveSlot0); }
    else if (Index == 1) { Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleSaveSlot1); }
    else if (Index == 2) { Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleSaveSlot2); }
}

void UManagementDashboardWidget::BindLoadButton(UButton* Button, int32 Index)
{
    if (Index == 0) { Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleLoadSlot0); }
    else if (Index == 1) { Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleLoadSlot1); }
    else if (Index == 2) { Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleLoadSlot2); }
}

void UManagementDashboardWidget::BindExtensionButton(UButton* Button, int32 Index)
{
    switch (Index) {
    case 0: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleExtend0); break;
    case 1: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleExtend1); break;
    case 2: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleExtend2); break;
    case 3: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleExtend3); break;
    case 4: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleExtend4); break;
    case 5: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleExtend5); break;
    case 6: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleExtend6); break;
    case 7: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleExtend7); break;
    default: break;
    }
}

void UManagementDashboardWidget::SelectTradeTeamAtSlot(int32 SlotIndex)
{
    if (SlotIndex < DisplayedTeamIds.Num())
    {
        SelectedTradeTeamId = DisplayedTeamIds[SlotIndex];
        TradeOutgoingPlayerIds.Reset();
        TradeIncomingPlayerIds.Reset();
        RefreshDashboard();
        SetScreen(7);
    }
}

void UManagementDashboardWidget::ToggleTradeOutgoingAtSlot(int32 SlotIndex)
{
    if (SlotIndex < DisplayedOutgoingIds.Num())
    {
        const FGuid& Id = DisplayedOutgoingIds[SlotIndex];
        if (TradeOutgoingPlayerIds.Contains(Id)) { TradeOutgoingPlayerIds.Remove(Id); }
        else { TradeOutgoingPlayerIds.Add(Id); }
        RefreshDashboard();
        SetScreen(7);
    }
}

void UManagementDashboardWidget::ToggleTradeIncomingAtSlot(int32 SlotIndex)
{
    if (SlotIndex < DisplayedIncomingIds.Num())
    {
        const FGuid& Id = DisplayedIncomingIds[SlotIndex];
        if (TradeIncomingPlayerIds.Contains(Id)) { TradeIncomingPlayerIds.Remove(Id); }
        else { TradeIncomingPlayerIds.Add(Id); }
        RefreshDashboard();
        SetScreen(7);
    }
}

void UManagementDashboardWidget::HandleTeam0() { SelectTradeTeamAtSlot(0); }
void UManagementDashboardWidget::HandleTeam1() { SelectTradeTeamAtSlot(1); }
void UManagementDashboardWidget::HandleTeam2() { SelectTradeTeamAtSlot(2); }
void UManagementDashboardWidget::HandleTeam3() { SelectTradeTeamAtSlot(3); }
void UManagementDashboardWidget::HandleTeam4() { SelectTradeTeamAtSlot(4); }
void UManagementDashboardWidget::HandleTeam5() { SelectTradeTeamAtSlot(5); }
void UManagementDashboardWidget::HandleTeam6() { SelectTradeTeamAtSlot(6); }
void UManagementDashboardWidget::HandleTeam7() { SelectTradeTeamAtSlot(7); }
void UManagementDashboardWidget::HandleOut0() { ToggleTradeOutgoingAtSlot(0); }
void UManagementDashboardWidget::HandleOut1() { ToggleTradeOutgoingAtSlot(1); }
void UManagementDashboardWidget::HandleOut2() { ToggleTradeOutgoingAtSlot(2); }
void UManagementDashboardWidget::HandleOut3() { ToggleTradeOutgoingAtSlot(3); }
void UManagementDashboardWidget::HandleOut4() { ToggleTradeOutgoingAtSlot(4); }
void UManagementDashboardWidget::HandleOut5() { ToggleTradeOutgoingAtSlot(5); }
void UManagementDashboardWidget::HandleOut6() { ToggleTradeOutgoingAtSlot(6); }
void UManagementDashboardWidget::HandleOut7() { ToggleTradeOutgoingAtSlot(7); }
void UManagementDashboardWidget::HandleIn0() { ToggleTradeIncomingAtSlot(0); }
void UManagementDashboardWidget::HandleIn1() { ToggleTradeIncomingAtSlot(1); }
void UManagementDashboardWidget::HandleIn2() { ToggleTradeIncomingAtSlot(2); }
void UManagementDashboardWidget::HandleIn3() { ToggleTradeIncomingAtSlot(3); }
void UManagementDashboardWidget::HandleIn4() { ToggleTradeIncomingAtSlot(4); }
void UManagementDashboardWidget::HandleIn5() { ToggleTradeIncomingAtSlot(5); }
void UManagementDashboardWidget::HandleIn6() { ToggleTradeIncomingAtSlot(6); }
void UManagementDashboardWidget::HandleIn7() { ToggleTradeIncomingAtSlot(7); }

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

void UManagementDashboardWidget::ApplyTactics(EPaceStyle Pace, EOffenseStyle Offense, EDefenseStyle Defense, EReboundPriority Rebounding)
{
    ULeagueGameSubsystem* Subsystem = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>();
    const FLeagueState League = Subsystem->GetLeague();
    if (League.Teams.Num() == 0) { return; }
    FString Error;
    if (Subsystem->SetTactics(League.Teams[0].TeamId, Pace, Offense, Defense, Rebounding, Error))
    { RefreshDashboard(); SetScreen(6); }
}

void UManagementDashboardWidget::SetTacticsPaceSlow()
{ const FLeagueState L = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>()->GetLeague(); if (L.Teams.Num()) { ApplyTactics(EPaceStyle::Slow, L.Teams[0].Tactics.Offense, L.Teams[0].Tactics.Defense, L.Teams[0].Tactics.Rebounding); } }
void UManagementDashboardWidget::SetTacticsPaceBalanced()
{ const FLeagueState L = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>()->GetLeague(); if (L.Teams.Num()) { ApplyTactics(EPaceStyle::Balanced, L.Teams[0].Tactics.Offense, L.Teams[0].Tactics.Defense, L.Teams[0].Tactics.Rebounding); } }
void UManagementDashboardWidget::SetTacticsPaceFast()
{ const FLeagueState L = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>()->GetLeague(); if (L.Teams.Num()) { ApplyTactics(EPaceStyle::Fast, L.Teams[0].Tactics.Offense, L.Teams[0].Tactics.Defense, L.Teams[0].Tactics.Rebounding); } }
void UManagementDashboardWidget::SetTacticsOffenseInside()
{ const FLeagueState L = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>()->GetLeague(); if (L.Teams.Num()) { ApplyTactics(L.Teams[0].Tactics.Pace, EOffenseStyle::Inside, L.Teams[0].Tactics.Defense, L.Teams[0].Tactics.Rebounding); } }
void UManagementDashboardWidget::SetTacticsOffenseBalanced()
{ const FLeagueState L = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>()->GetLeague(); if (L.Teams.Num()) { ApplyTactics(L.Teams[0].Tactics.Pace, EOffenseStyle::Balanced, L.Teams[0].Tactics.Defense, L.Teams[0].Tactics.Rebounding); } }
void UManagementDashboardWidget::SetTacticsOffensePerimeter()
{ const FLeagueState L = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>()->GetLeague(); if (L.Teams.Num()) { ApplyTactics(L.Teams[0].Tactics.Pace, EOffenseStyle::Perimeter, L.Teams[0].Tactics.Defense, L.Teams[0].Tactics.Rebounding); } }
void UManagementDashboardWidget::SetTacticsDefenseMan()
{ const FLeagueState L = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>()->GetLeague(); if (L.Teams.Num()) { ApplyTactics(L.Teams[0].Tactics.Pace, L.Teams[0].Tactics.Offense, EDefenseStyle::Man, L.Teams[0].Tactics.Rebounding); } }
void UManagementDashboardWidget::SetTacticsDefenseSwitching()
{ const FLeagueState L = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>()->GetLeague(); if (L.Teams.Num()) { ApplyTactics(L.Teams[0].Tactics.Pace, L.Teams[0].Tactics.Offense, EDefenseStyle::Switching, L.Teams[0].Tactics.Rebounding); } }
void UManagementDashboardWidget::SetTacticsDefenseZone()
{ const FLeagueState L = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>()->GetLeague(); if (L.Teams.Num()) { ApplyTactics(L.Teams[0].Tactics.Pace, L.Teams[0].Tactics.Offense, EDefenseStyle::Zone, L.Teams[0].Tactics.Rebounding); } }
void UManagementDashboardWidget::SetTacticsReboundTransition()
{ const FLeagueState L = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>()->GetLeague(); if (L.Teams.Num()) { ApplyTactics(L.Teams[0].Tactics.Pace, L.Teams[0].Tactics.Offense, L.Teams[0].Tactics.Defense, EReboundPriority::Transition); } }
void UManagementDashboardWidget::SetTacticsReboundBalanced()
{ const FLeagueState L = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>()->GetLeague(); if (L.Teams.Num()) { ApplyTactics(L.Teams[0].Tactics.Pace, L.Teams[0].Tactics.Offense, L.Teams[0].Tactics.Defense, EReboundPriority::Balanced); } }
void UManagementDashboardWidget::SetTacticsReboundCrash()
{ const FLeagueState L = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>()->GetLeague(); if (L.Teams.Num()) { ApplyTactics(L.Teams[0].Tactics.Pace, L.Teams[0].Tactics.Offense, L.Teams[0].Tactics.Defense, EReboundPriority::CrashBoards); } }

void UManagementDashboardWidget::HandleSignFreeAgent()
{
    ULeagueGameSubsystem* Subsystem = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>();
    const FLeagueState League = Subsystem->GetLeague();
    if (League.Teams.Num() == 0) { return; }
    const TArray<FFreeAgent>& Pool = League.Offseason.FreeAgentPool;
    int32 BestIndex = -1;
    for (int32 Index = 0; Index < Pool.Num(); ++Index)
    {
        if (!Pool[Index].bSigned) { BestIndex = Index; break; }
    }
    if (BestIndex < 0) { return; }
    FString Error;
    const FFreeAgent& FA = Pool[BestIndex];
    if (Subsystem->SignFreeAgent(League.Teams[0].TeamId, BestIndex, FA.AskingSalary, 2, Error))
    {
        RefreshDashboard();
        SetScreen(11);
        StatusText->SetText(FText::FromString(FString::Printf(TEXT("Signed %s!"), *FA.Profile.DisplayName)));
        StatusText->SetColorAndOpacity(FSlateColor(DashboardStyle::Success));
    }
    else
    {
        StatusText->SetText(FText::FromString(Error));
        StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.32f, 0.28f, 1.0f)));
    }
}

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
            SetScreen(11);
        }
        else
        {
            StatusText->SetText(FText::FromString(Error));
            StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.32f, 0.28f, 1.0f)));
        }
        return;
    }

    const FLeagueState PreLeague = LeagueSubsystem->GetLeague();
    if (PreLeague.Teams.Num() == 0) { return; }
    const FGuid& ClubId = PreLeague.Teams[0].TeamId;
    PendingMatchGameId = FGuid();

    if (PreLeague.Phase == ESeasonPhase::RegularSeason)
    {
        for (const FScheduledGame& Game : PreLeague.Schedule)
        {
            if (!Game.bComplete && Game.Round == PreLeague.CurrentRound &&
                (Game.HomeTeamId == ClubId || Game.AwayTeamId == ClubId))
            {
                PendingMatchGameId = Game.GameId;
                break;
            }
        }
    }

    if (PendingMatchGameId.IsValid())
    {
        RefreshDashboard();
        SetScreen(17);
        StatusText->SetText(FText::FromString(TEXT("Match Center — choose how to experience the game")));
        StatusText->SetColorAndOpacity(FSlateColor(DashboardStyle::Accent));
    }
    else
    {
        ExecuteSimulateRound();
    }
}

void UManagementDashboardWidget::ExecuteSimulateRound()
{
    ULeagueGameSubsystem* LeagueSubsystem = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>();
    if (!LeagueSubsystem) { return; }
    FString Error;

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

        for (int32 I = 0; I < LastRoundResults.Num() && I < LastRoundSnapshots.Num(); ++I)
        {
            const FMatchSnapshot& Snap = LastRoundSnapshots[I];
            if (Snap.HomeTeam.TeamId == PostLeague.Teams[0].TeamId ||
                Snap.AwayTeam.TeamId == PostLeague.Teams[0].TeamId)
            {
                LastPresentation = LeagueSubsystem->BuildPresentationPackage(LastRoundResults[I], Snap);
                break;
            }
        }

        FString AutoSaveError;
        LeagueSubsystem->SaveLeagueAsync(TEXT("AutoSave"), AutoSaveError);
        RefreshDashboard();
        const FString PhaseLabel = LeagueSubsystem->GetSeasonPhase() == ESeasonPhase::Playoffs
            ? TEXT("Playoff") : TEXT("Round");
        StatusText->SetText(FText::FromString(FString::Printf(
            TEXT("%s complete  •  %d games resolved  •  Auto-saved"), *PhaseLabel, Results.Num())));
        StatusText->SetColorAndOpacity(FSlateColor(DashboardStyle::Success));
        SetScreen(9);
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
        case EOffseasonStep::FreeAgency: return TEXT("FREE AGENCY");
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

    if (Offseason.CurrentStep == EOffseasonStep::FreeAgency && Offseason.FreeAgentPool.Num() > 0)
    {
        int32 Available = 0;
        for (const FFreeAgent& FA : Offseason.FreeAgentPool) { Available += FA.bSigned ? 0 : 1; }
        OffseasonList->AddChildToVerticalBox(MakeText(
            FString::Printf(TEXT("FREE AGENT MARKET  •  %d AVAILABLE"), Available),
            11, DashboardStyle::Primary, true))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

        int32 Shown = 0;
        for (int32 Index = 0; Index < Offseason.FreeAgentPool.Num() && Shown < 8; ++Index)
        {
            const FFreeAgent& FA = Offseason.FreeAgentPool[Index];
            if (FA.bSigned) { continue; }
            Shown++;
            UBorder* Row = MakeCard(DashboardStyle::CardRaised);
            Row->SetPadding(FMargin(12.0f, 6.0f));
            UHorizontalBox* RowContent = WidgetTree->ConstructWidget<UHorizontalBox>();
            Row->SetContent(RowContent);
            RowContent->AddChildToHorizontalBox(MakeText(FA.Profile.DisplayName, 11,
                DashboardStyle::Primary, true))->SetSize(DashboardStyle::Size(1.0f));
            RowContent->AddChildToHorizontalBox(MakeText(DashboardStyle::Position(FA.Profile.Position), 10,
                DashboardStyle::Accent, true))->SetSize(DashboardStyle::Size(0.2f));
            RowContent->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("OVR %d"), FA.Profile.Ratings.Overall()),
                10, DashboardStyle::Primary, true))->SetSize(DashboardStyle::Size(0.3f));
            RowContent->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("AGE %d"), FA.Profile.Age),
                10, DashboardStyle::Secondary))->SetSize(DashboardStyle::Size(0.25f));
            UTextBlock* Salary = MakeText(FString::Printf(TEXT("$%.1fM"), FA.AskingSalary / 100000000.0),
                9, DashboardStyle::Accent);
            Salary->SetJustification(ETextJustify::Right);
            RowContent->AddChildToHorizontalBox(Salary)->SetSize(DashboardStyle::Size(0.4f));
            OffseasonList->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 3.0f));
        }

        if (Club.Players.Num() < 15 && Available > 0)
        {
            UButton* SignButton = WidgetTree->ConstructWidget<UButton>();
            SignButton->SetBackgroundColor(DashboardStyle::Accent);
            SignButton->SetContent(MakeText(TEXT("SIGN BEST AVAILABLE FREE AGENT"), 11, DashboardStyle::Background, true));
            SignButton->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleSignFreeAgent);
            OffseasonList->AddChildToVerticalBox(SignButton)->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 10.0f));
        }
    }

    if (Offseason.CurrentStep == EOffseasonStep::Draft && Offseason.DraftClass.Num() > 0)
    {
        bool bPlayerHasDrafted = false;
        for (const FDraftProspect& P : Offseason.DraftClass)
        {
            if (P.bDrafted && P.DraftedByTeamId == Club.TeamId) { bPlayerHasDrafted = true; break; }
        }

        OffseasonList->AddChildToVerticalBox(MakeText(
            FString::Printf(TEXT("DRAFT CLASS  •  PICK %d"), Offseason.CurrentDraftPick + 1),
            11, DashboardStyle::Primary, true))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));

        if (!bPlayerHasDrafted)
        {
            OffseasonList->AddChildToVerticalBox(MakeText(
                TEXT("Select a prospect to draft. Click ADVANCE when done to auto-draft remaining picks."),
                10, DashboardStyle::Success))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
        }

        DisplayedDraftIndices.Reset();
        int32 DraftSlot = 0;
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
                ->SetSize(DashboardStyle::Size(0.8f));
            RowContent->AddChildToHorizontalBox(MakeText(DashboardStyle::Position(Prospect.Profile.Position), 10,
                DashboardStyle::Accent, true))->SetSize(DashboardStyle::Size(0.18f));
            RowContent->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("OVR %d"), Prospect.Profile.Ratings.Overall()),
                10, DashboardStyle::Primary, true))->SetSize(DashboardStyle::Size(0.28f));
            RowContent->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("AGE %d"), Prospect.Profile.Age),
                10, DashboardStyle::Secondary))->SetSize(DashboardStyle::Size(0.22f));
            if (Prospect.bDrafted)
            {
                const FTeamState* DraftedBy = League.Teams.FindByPredicate(
                    [&Prospect](const FTeamState& T) { return T.TeamId == Prospect.DraftedByTeamId; });
                UTextBlock* DraftedText = MakeText(
                    DraftedBy ? DraftedBy->Nickname : TEXT("???"), 9, DashboardStyle::Secondary);
                DraftedText->SetJustification(ETextJustify::Right);
                RowContent->AddChildToHorizontalBox(DraftedText)->SetSize(DashboardStyle::Size(0.35f));
            }
            else if (!bPlayerHasDrafted && DraftSlot < 8)
            {
                DisplayedDraftIndices.Add(Index);
                UButton* DraftBtn = WidgetTree->ConstructWidget<UButton>();
                DraftBtn->SetBackgroundColor(DashboardStyle::Accent);
                DraftBtn->SetContent(MakeText(TEXT("DRAFT"), 9, DashboardStyle::Background, true));
                BindDraftButton(DraftBtn, DraftSlot);
                RowContent->AddChildToHorizontalBox(DraftBtn)->SetSize(DashboardStyle::Size(0.35f));
                DraftSlot++;
            }
            else
            {
                UTextBlock* AvailText = MakeText(TEXT("AVAILABLE"), 9, DashboardStyle::Success, true);
                AvailText->SetJustification(ETextJustify::Right);
                RowContent->AddChildToHorizontalBox(AvailText)->SetSize(DashboardStyle::Size(0.35f));
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
            SetScreen(11);
        }
    }
    else
    {
        StatusText->SetText(FText::FromString(Error));
        StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.32f, 0.28f, 1.0f)));
    }
}

void UManagementDashboardWidget::DraftProspectAtSlot(int32 SlotIndex)
{
    if (SlotIndex >= DisplayedDraftIndices.Num()) { return; }
    ULeagueGameSubsystem* Subsystem = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>();
    const FLeagueState League = Subsystem->GetLeague();
    if (League.Teams.Num() == 0) { return; }
    FString Error;
    if (Subsystem->DraftPlayer(League.Teams[0].TeamId, DisplayedDraftIndices[SlotIndex], Error))
    {
        RefreshDashboard();
        SetScreen(11);
        const FDraftProspect& Drafted = Subsystem->GetOffseasonState().DraftClass[DisplayedDraftIndices[SlotIndex]];
        StatusText->SetText(FText::FromString(FString::Printf(TEXT("Drafted %s!"), *Drafted.Profile.DisplayName)));
        StatusText->SetColorAndOpacity(FSlateColor(DashboardStyle::Success));
    }
    else
    {
        StatusText->SetText(FText::FromString(Error));
        StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.32f, 0.28f, 1.0f)));
    }
}

void UManagementDashboardWidget::HandleDraft0() { DraftProspectAtSlot(0); }
void UManagementDashboardWidget::HandleDraft1() { DraftProspectAtSlot(1); }
void UManagementDashboardWidget::HandleDraft2() { DraftProspectAtSlot(2); }
void UManagementDashboardWidget::HandleDraft3() { DraftProspectAtSlot(3); }
void UManagementDashboardWidget::HandleDraft4() { DraftProspectAtSlot(4); }
void UManagementDashboardWidget::HandleDraft5() { DraftProspectAtSlot(5); }
void UManagementDashboardWidget::HandleDraft6() { DraftProspectAtSlot(6); }
void UManagementDashboardWidget::HandleDraft7() { DraftProspectAtSlot(7); }

void UManagementDashboardWidget::RefreshSaveLoadScreen()
{
    SaveLoadList->ClearChildren();

    ULeagueGameSubsystem* Subsystem = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>();
    if (!Subsystem) { return; }

    const FString SlotNames[] = { TEXT("SaveSlot1"), TEXT("SaveSlot2"), TEXT("SaveSlot3") };
    const FString SlotLabels[] = { TEXT("SLOT 1"), TEXT("SLOT 2"), TEXT("SLOT 3") };

    SaveLoadList->AddChildToVerticalBox(MakeText(TEXT("AUTO-SAVE"), 10,
        DashboardStyle::Accent, true))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));

    {
        int32 Season = 0, Round = 0;
        FString TeamName;
        FDateTime SavedAt;
        UBorder* AutoCard = MakeCard(DashboardStyle::CardRaised);
        AutoCard->SetPadding(FMargin(14.0f, 10.0f));
        UVerticalBox* AutoContent = WidgetTree->ConstructWidget<UVerticalBox>();
        AutoCard->SetContent(AutoContent);
        if (Subsystem->GetSaveSlotInfo(TEXT("AutoSave"), Season, Round, TeamName, SavedAt))
        {
            AutoContent->AddChildToVerticalBox(MakeText(
                FString::Printf(TEXT("%s  •  Season %d  •  Round %d"), *TeamName, Season, Round + 1),
                11, DashboardStyle::Primary, true));
            AutoContent->AddChildToVerticalBox(MakeText(
                FString::Printf(TEXT("Saved: %s"), *SavedAt.ToString(TEXT("%Y-%m-%d %H:%M"))),
                9, DashboardStyle::Secondary))->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
        }
        else
        {
            AutoContent->AddChildToVerticalBox(MakeText(TEXT("No auto-save data"),
                11, DashboardStyle::Secondary));
        }
        SaveLoadList->AddChildToVerticalBox(AutoCard)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
    }

    SaveLoadList->AddChildToVerticalBox(MakeText(TEXT("SAVE SLOTS"), 10,
        DashboardStyle::Accent, true))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));

    for (int32 SlotIdx = 0; SlotIdx < 3; ++SlotIdx)
    {
        int32 Season = 0, Round = 0;
        FString TeamName;
        FDateTime SavedAt;
        const bool bExists = Subsystem->GetSaveSlotInfo(SlotNames[SlotIdx], Season, Round, TeamName, SavedAt);

        UBorder* SlotCard = MakeCard(DashboardStyle::CardRaised);
        SlotCard->SetPadding(FMargin(14.0f, 10.0f));
        UVerticalBox* SlotContent = WidgetTree->ConstructWidget<UVerticalBox>();
        SlotCard->SetContent(SlotContent);

        SlotContent->AddChildToVerticalBox(MakeText(SlotLabels[SlotIdx], 12,
            DashboardStyle::Primary, true));

        if (bExists)
        {
            SlotContent->AddChildToVerticalBox(MakeText(
                FString::Printf(TEXT("%s  •  Season %d  •  Round %d"), *TeamName, Season, Round + 1),
                10, DashboardStyle::Secondary))->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
            SlotContent->AddChildToVerticalBox(MakeText(
                FString::Printf(TEXT("Saved: %s"), *SavedAt.ToString(TEXT("%Y-%m-%d %H:%M"))),
                9, DashboardStyle::Secondary))->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
        }
        else
        {
            SlotContent->AddChildToVerticalBox(MakeText(TEXT("EMPTY"),
                10, DashboardStyle::Secondary))->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
        }

        UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>();
        UButton* SaveBtn = WidgetTree->ConstructWidget<UButton>();
        SaveBtn->SetBackgroundColor(DashboardStyle::Accent);
        SaveBtn->SetContent(MakeText(TEXT("SAVE"), 10, DashboardStyle::Background, true));
        BindSaveButton(SaveBtn, SlotIdx);
        Buttons->AddChildToHorizontalBox(SaveBtn)->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));

        if (bExists)
        {
            UButton* LoadBtn = WidgetTree->ConstructWidget<UButton>();
            LoadBtn->SetBackgroundColor(DashboardStyle::CardRaised);
            LoadBtn->SetContent(MakeText(TEXT("LOAD"), 10, DashboardStyle::Primary, true));
            BindLoadButton(LoadBtn, SlotIdx);
            Buttons->AddChildToHorizontalBox(LoadBtn);
        }

        SlotContent->AddChildToVerticalBox(Buttons)->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
        SaveLoadList->AddChildToVerticalBox(SlotCard)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
    }
}

void UManagementDashboardWidget::SaveToSlot(int32 SlotIndex)
{
    const FString SlotNames[] = { TEXT("SaveSlot1"), TEXT("SaveSlot2"), TEXT("SaveSlot3") };
    if (SlotIndex < 0 || SlotIndex > 2) { return; }
    ULeagueGameSubsystem* Subsystem = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>();
    if (!Subsystem) { return; }
    FString Error;
    if (Subsystem->SaveLeagueAsync(SlotNames[SlotIndex], Error))
    {
        RefreshDashboard();
        SetScreen(12);
        StatusText->SetText(FText::FromString(FString::Printf(TEXT("Saved to Slot %d"), SlotIndex + 1)));
        StatusText->SetColorAndOpacity(FSlateColor(DashboardStyle::Success));
    }
    else
    {
        StatusText->SetText(FText::FromString(Error));
        StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.32f, 0.28f, 1.0f)));
    }
}

void UManagementDashboardWidget::LoadFromSlot(int32 SlotIndex)
{
    const FString SlotNames[] = { TEXT("SaveSlot1"), TEXT("SaveSlot2"), TEXT("SaveSlot3") };
    if (SlotIndex < 0 || SlotIndex > 2) { return; }
    ULeagueGameSubsystem* Subsystem = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>();
    if (!Subsystem) { return; }
    FString Error;
    if (Subsystem->LoadLeague(SlotNames[SlotIndex], Error))
    {
        RefreshDashboard();
        SetScreen(0);
        StatusText->SetText(FText::FromString(FString::Printf(TEXT("Loaded from Slot %d"), SlotIndex + 1)));
        StatusText->SetColorAndOpacity(FSlateColor(DashboardStyle::Success));
    }
    else
    {
        StatusText->SetText(FText::FromString(Error));
        StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.32f, 0.28f, 1.0f)));
    }
}

void UManagementDashboardWidget::HandleSaveSlot0() { SaveToSlot(0); }
void UManagementDashboardWidget::HandleSaveSlot1() { SaveToSlot(1); }
void UManagementDashboardWidget::HandleSaveSlot2() { SaveToSlot(2); }
void UManagementDashboardWidget::HandleLoadSlot0() { LoadFromSlot(0); }
void UManagementDashboardWidget::HandleLoadSlot1() { LoadFromSlot(1); }
void UManagementDashboardWidget::HandleLoadSlot2() { LoadFromSlot(2); }

void UManagementDashboardWidget::RefreshGameRecapScreen(const FLeagueState& League, const FTeamState& Club)
{
    GameRecapList->ClearChildren();

    if (LastRoundResults.Num() == 0 || LastRoundSnapshots.Num() == 0)
    {
        GameRecapList->AddChildToVerticalBox(MakeText(
            TEXT("Simulate a round to see the game recap here."), 12, DashboardStyle::Secondary));
        return;
    }

    ULeagueGameSubsystem* Subsystem = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>();

    for (int32 GameIdx = 0; GameIdx < LastRoundResults.Num() && GameIdx < LastRoundSnapshots.Num(); ++GameIdx)
    {
        const FMatchResult& Result = LastRoundResults[GameIdx];
        const FMatchSnapshot& Snapshot = LastRoundSnapshots[GameIdx];

        const bool bPlayerGame = Snapshot.HomeTeam.TeamId == Club.TeamId || Snapshot.AwayTeam.TeamId == Club.TeamId;
        if (!bPlayerGame && GameIdx > 0) { continue; }

        FGameRecap Recap = Subsystem->BuildGameRecap(Result, Snapshot);

        UBorder* GameCard = MakeCard(bPlayerGame ? DashboardStyle::CardRaised : DashboardStyle::Card);
        GameCard->SetPadding(FMargin(14.0f, 12.0f));
        UVerticalBox* GameContent = WidgetTree->ConstructWidget<UVerticalBox>();
        GameCard->SetContent(GameContent);

        GameContent->AddChildToVerticalBox(MakeText(
            FString::Printf(TEXT("%s  vs  %s"), *Recap.HomeTeamName, *Recap.AwayTeamName),
            14, DashboardStyle::Primary, true));

        GameContent->AddChildToVerticalBox(MakeText(
            FString::Printf(TEXT("FINAL: %d - %d"), Recap.FinalHomeScore, Recap.FinalAwayScore),
            18, DashboardStyle::Accent, true))->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));

        if (Recap.QuarterScores.Num() > 0)
        {
            UHorizontalBox* QuarterRow = WidgetTree->ConstructWidget<UHorizontalBox>();
            QuarterRow->AddChildToHorizontalBox(MakeText(TEXT(""), 9, DashboardStyle::Secondary))
                ->SetSize(DashboardStyle::Size(0.4f));
            for (int32 Q = 0; Q < Recap.QuarterScores.Num(); ++Q)
            {
                FString QLabel = Q < 4 ? FString::Printf(TEXT("Q%d"), Q + 1) : FString::Printf(TEXT("OT%d"), Q - 3);
                QuarterRow->AddChildToHorizontalBox(MakeText(QLabel, 9, DashboardStyle::Secondary, true))
                    ->SetSize(DashboardStyle::Size(0.15f));
            }
            GameContent->AddChildToVerticalBox(QuarterRow)->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 2.0f));

            auto AddScoreLine = [&](const FString& TeamName, bool bHome)
            {
                UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
                Row->AddChildToHorizontalBox(MakeText(TeamName, 10, DashboardStyle::Primary, true))
                    ->SetSize(DashboardStyle::Size(0.4f));
                for (int32 Q = 0; Q < Recap.QuarterScores.Num(); ++Q)
                {
                    const int32 Pts = bHome ? Recap.QuarterScores[Q].HomePoints : Recap.QuarterScores[Q].AwayPoints;
                    Row->AddChildToHorizontalBox(MakeText(FString::FromInt(Pts), 10, DashboardStyle::Primary))
                        ->SetSize(DashboardStyle::Size(0.15f));
                }
                GameContent->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
            };
            AddScoreLine(Recap.HomeTeamName, true);
            AddScoreLine(Recap.AwayTeamName, false);
        }

        if (bPlayerGame)
        {
            GameContent->AddChildToVerticalBox(MakeText(TEXT("HIGHLIGHTS"), 10, DashboardStyle::Accent, true))
                ->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 4.0f));

            int32 HighlightCount = 0;
            for (const FPlayByPlayEntry& Entry : Recap.PlayByPlay)
            {
                if (!Entry.bHighlight || HighlightCount >= 12) { continue; }
                HighlightCount++;

                FString TimeStr = Entry.Period <= 4
                    ? FString::Printf(TEXT("Q%d %d:%02d"), Entry.Period, Entry.ClockSeconds / 60, Entry.ClockSeconds % 60)
                    : FString::Printf(TEXT("OT%d %d:%02d"), Entry.Period - 4, Entry.ClockSeconds / 60, Entry.ClockSeconds % 60);

                UHorizontalBox* PlayRow = WidgetTree->ConstructWidget<UHorizontalBox>();
                PlayRow->AddChildToHorizontalBox(MakeText(TimeStr, 9, DashboardStyle::Secondary))
                    ->SetSize(DashboardStyle::Size(0.18f));
                PlayRow->AddChildToHorizontalBox(MakeText(Entry.Description, 9, DashboardStyle::Primary))
                    ->SetSize(DashboardStyle::Size(0.62f));
                PlayRow->AddChildToHorizontalBox(MakeText(
                    FString::Printf(TEXT("%d-%d"), Entry.HomeScore, Entry.AwayScore), 9, DashboardStyle::Accent))
                    ->SetSize(DashboardStyle::Size(0.2f));
                GameContent->AddChildToVerticalBox(PlayRow)->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
            }
        }

        GameRecapList->AddChildToVerticalBox(GameCard)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
    }
}

void UManagementDashboardWidget::RefreshHistoryScreen(const FLeagueState& League)
{
    HistoryList->ClearChildren();

    const FLeagueHistory& History = League.History;

    if (History.Championships.Num() > 0)
    {
        HistoryList->AddChildToVerticalBox(MakeText(TEXT("CHAMPIONSHIP BANNERS"), 10,
            DashboardStyle::Accent, true))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));

        for (int32 I = History.Championships.Num() - 1; I >= 0 && I >= History.Championships.Num() - 10; --I)
        {
            const FChampionshipRecord& Record = History.Championships[I];
            UBorder* Banner = MakeCard(DashboardStyle::CardRaised);
            Banner->SetPadding(FMargin(14.0f, 10.0f));
            UVerticalBox* BContent = WidgetTree->ConstructWidget<UVerticalBox>();
            Banner->SetContent(BContent);

            BContent->AddChildToVerticalBox(MakeText(
                FString::Printf(TEXT("Season %d Champion"), Record.Season),
                12, DashboardStyle::Accent, true));
            BContent->AddChildToVerticalBox(MakeText(Record.ChampionName, 16, DashboardStyle::Primary, true))
                ->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
            if (!Record.MvpName.IsEmpty())
            {
                BContent->AddChildToVerticalBox(MakeText(
                    FString::Printf(TEXT("Finals MVP: %s"), *Record.MvpName),
                    10, DashboardStyle::Secondary))->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
            }
            HistoryList->AddChildToVerticalBox(Banner)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
        }
    }
    else
    {
        HistoryList->AddChildToVerticalBox(MakeText(
            TEXT("Complete a season to begin building league history."), 12, DashboardStyle::Secondary));
    }

    if (History.AllTimeLeaders.Num() > 0)
    {
        HistoryList->AddChildToVerticalBox(MakeText(TEXT("ALL-TIME SCORING LEADERS"), 10,
            DashboardStyle::Accent, true))->SetPadding(FMargin(0.0f, 16.0f, 0.0f, 6.0f));

        TArray<FAllTimeLeader> TopScorers = FLeagueHistoryService::GetTopScorers(History, 8);
        for (int32 I = 0; I < TopScorers.Num(); ++I)
        {
            const FAllTimeLeader& Leader = TopScorers[I];
            UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
            Row->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("%d."), I + 1), 10, DashboardStyle::Secondary))
                ->SetSize(DashboardStyle::Size(0.06f));
            Row->AddChildToHorizontalBox(MakeText(Leader.PlayerName, 10, DashboardStyle::Primary, true))
                ->SetSize(DashboardStyle::Size(0.34f));
            Row->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("%d pts"), Leader.TotalPoints), 10, DashboardStyle::Accent))
                ->SetSize(DashboardStyle::Size(0.2f));
            Row->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("%d reb"), Leader.TotalRebounds), 10, DashboardStyle::Secondary))
                ->SetSize(DashboardStyle::Size(0.2f));
            Row->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("%d ast"), Leader.TotalAssists), 10, DashboardStyle::Secondary))
                ->SetSize(DashboardStyle::Size(0.2f));
            HistoryList->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 0.0f));
        }

        HistoryList->AddChildToVerticalBox(MakeText(TEXT("ALL-TIME ASSISTS LEADERS"), 10,
            DashboardStyle::Accent, true))->SetPadding(FMargin(0.0f, 16.0f, 0.0f, 6.0f));

        TArray<FAllTimeLeader> TopAssisters = FLeagueHistoryService::GetTopAssisters(History, 5);
        for (int32 I = 0; I < TopAssisters.Num(); ++I)
        {
            const FAllTimeLeader& Leader = TopAssisters[I];
            UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
            Row->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("%d."), I + 1), 10, DashboardStyle::Secondary))
                ->SetSize(DashboardStyle::Size(0.06f));
            Row->AddChildToHorizontalBox(MakeText(Leader.PlayerName, 10, DashboardStyle::Primary, true))
                ->SetSize(DashboardStyle::Size(0.34f));
            Row->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("%d ast"), Leader.TotalAssists), 10, DashboardStyle::Accent))
                ->SetSize(DashboardStyle::Size(0.2f));
            Row->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("%d GP"), Leader.GamesPlayed), 10, DashboardStyle::Secondary))
                ->SetSize(DashboardStyle::Size(0.2f));
            Row->AddChildToHorizontalBox(MakeText(
                Leader.MvpAwards > 0 ? FString::Printf(TEXT("%dx MVP"), Leader.MvpAwards) : TEXT(""),
                10, DashboardStyle::Accent))->SetSize(DashboardStyle::Size(0.2f));
            HistoryList->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 0.0f));
        }
    }
}

void UManagementDashboardWidget::RefreshContractsScreen(const FLeagueState& League, const FTeamState& Club)
{
    ContractsList->ClearChildren();
    DisplayedExtensionPlayerIds.Reset();

    ULeagueGameSubsystem* Subsystem = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>();
    TArray<FExtensionOffer> Eligible = Subsystem->GetEligibleExtensions(Club.TeamId);

    if (Eligible.Num() == 0)
    {
        ContractsList->AddChildToVerticalBox(MakeText(
            TEXT("No players are currently eligible for contract extensions."), 12, DashboardStyle::Secondary));
        ContractsList->AddChildToVerticalBox(MakeText(
            TEXT("Players with 1 year or less remaining become eligible."), 10, DashboardStyle::Secondary))
            ->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));
        return;
    }

    ContractsList->AddChildToVerticalBox(MakeText(
        TEXT("Offer extensions to players before they hit free agency."), 10, DashboardStyle::Secondary))
        ->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));

    for (int32 I = 0; I < Eligible.Num() && I < 8; ++I)
    {
        const FExtensionOffer& Offer = Eligible[I];
        DisplayedExtensionPlayerIds.Add(Offer.PlayerId);

        const FPlayerProfile* Player = Club.Players.FindByPredicate(
            [&Offer](const FPlayerProfile& P) { return P.PlayerId == Offer.PlayerId; });
        if (!Player) { continue; }

        UBorder* Card = MakeCard(DashboardStyle::Card);
        Card->SetPadding(FMargin(14.0f, 10.0f));
        UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
        Card->SetContent(Content);

        UHorizontalBox* TopRow = WidgetTree->ConstructWidget<UHorizontalBox>();
        TopRow->AddChildToHorizontalBox(MakeText(Player->DisplayName, 13, DashboardStyle::Primary, true))
            ->SetSize(DashboardStyle::Size(0.45f));
        TopRow->AddChildToHorizontalBox(MakeText(
            FString::Printf(TEXT("%s  •  OVR %d  •  Age %d"),
                *DashboardStyle::Position(Player->Position), Player->Ratings.Overall(), Player->Age),
            10, DashboardStyle::Secondary))->SetSize(DashboardStyle::Size(0.55f));
        Content->AddChildToVerticalBox(TopRow);

        UHorizontalBox* ContractRow = WidgetTree->ConstructWidget<UHorizontalBox>();
        ContractRow->AddChildToHorizontalBox(MakeText(
            FString::Printf(TEXT("Current: $%lldM / %dy remaining"),
                Player->Contract.SalaryMinorUnits / 1000000, Player->Contract.YearsRemaining),
            10, DashboardStyle::Secondary))->SetSize(DashboardStyle::Size(0.5f));
        ContractRow->AddChildToHorizontalBox(MakeText(
            FString::Printf(TEXT("Asking: ~$%lldM / %dy"),
                Offer.AskingSalary / 1000000, Offer.AskingYears),
            10, DashboardStyle::Accent))->SetSize(DashboardStyle::Size(0.5f));
        Content->AddChildToVerticalBox(ContractRow)->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));

        UButton* ExtendBtn = WidgetTree->ConstructWidget<UButton>();
        ExtendBtn->SetBackgroundColor(DashboardStyle::Accent);
        ExtendBtn->SetContent(MakeText(
            FString::Printf(TEXT("OFFER $%lldM / %dy"), Offer.AskingSalary / 1000000, Offer.AskingYears),
            10, DashboardStyle::Background, true));
        BindExtensionButton(ExtendBtn, I);
        Content->AddChildToVerticalBox(ExtendBtn)->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));

        ContractsList->AddChildToVerticalBox(Card)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
    }

    ContractsList->AddChildToVerticalBox(MakeText(
        FString::Printf(TEXT("Team Salary: $%lldM / $%lldM cap"),
            Club.TotalSalary() / 1000000, Club.SalaryCapMinorUnits / 1000000),
        10, Club.IsOverCap() ? FLinearColor(0.95f, 0.32f, 0.28f, 1.0f) : DashboardStyle::Secondary))
        ->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
}

void UManagementDashboardWidget::OfferExtensionAtSlot(int32 SlotIndex)
{
    if (SlotIndex >= DisplayedExtensionPlayerIds.Num()) { return; }
    ULeagueGameSubsystem* Subsystem = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>();
    const FLeagueState League = Subsystem->GetLeague();
    if (League.Teams.Num() == 0) { return; }

    const FGuid& PlayerId = DisplayedExtensionPlayerIds[SlotIndex];
    const FPlayerProfile* Player = League.Teams[0].Players.FindByPredicate(
        [&PlayerId](const FPlayerProfile& P) { return P.PlayerId == PlayerId; });
    if (!Player) { return; }

    const FAthleteState* State = League.Teams[0].PlayerStates.FindByPredicate(
        [&PlayerId](const FAthleteState& S) { return S.PlayerId == PlayerId; });
    FExtensionOffer Asking = FContractService::CalculateAskingPrice(*Player, State);

    FString Error;
    if (Subsystem->OfferExtension(League.Teams[0].TeamId, PlayerId, Asking.AskingSalary, Asking.AskingYears, Error))
    {
        RefreshDashboard();
        SetScreen(15);
        StatusText->SetText(FText::FromString(FString::Printf(TEXT("%s signed a %dy extension!"), *Player->DisplayName, Asking.AskingYears)));
        StatusText->SetColorAndOpacity(FSlateColor(DashboardStyle::Success));
    }
    else
    {
        StatusText->SetText(FText::FromString(Error));
        StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.32f, 0.28f, 1.0f)));
    }
}

void UManagementDashboardWidget::HandleExtend0() { OfferExtensionAtSlot(0); }
void UManagementDashboardWidget::HandleExtend1() { OfferExtensionAtSlot(1); }
void UManagementDashboardWidget::HandleExtend2() { OfferExtensionAtSlot(2); }
void UManagementDashboardWidget::HandleExtend3() { OfferExtensionAtSlot(3); }
void UManagementDashboardWidget::HandleExtend4() { OfferExtensionAtSlot(4); }
void UManagementDashboardWidget::HandleExtend5() { OfferExtensionAtSlot(5); }
void UManagementDashboardWidget::HandleExtend6() { OfferExtensionAtSlot(6); }
void UManagementDashboardWidget::HandleExtend7() { OfferExtensionAtSlot(7); }

void UManagementDashboardWidget::RefreshRivalriesScreen(const FLeagueState& League, const FTeamState& Club)
{
    RivalriesList->ClearChildren();

    TArray<FRivalry> TopRivalries = FRivalryService::GetTopRivalries(League, 8);

    if (TopRivalries.Num() == 0)
    {
        RivalriesList->AddChildToVerticalBox(MakeText(
            TEXT("No rivalries have formed yet. Play close games and meet in the playoffs to build intensity."),
            12, DashboardStyle::Secondary));
        return;
    }

    auto FindTeamName = [&League](const FGuid& TeamId) -> FString
    {
        const FTeamState* Team = League.Teams.FindByPredicate(
            [&TeamId](const FTeamState& T) { return T.TeamId == TeamId; });
        return Team ? Team->City + TEXT(" ") + Team->Nickname : TEXT("Unknown");
    };

    for (const FRivalry& Rivalry : TopRivalries)
    {
        const bool bInvolvesPlayer = Rivalry.TeamAId == Club.TeamId || Rivalry.TeamBId == Club.TeamId;
        UBorder* Card = MakeCard(bInvolvesPlayer ? DashboardStyle::CardRaised : DashboardStyle::Card);
        Card->SetPadding(FMargin(14.0f, 10.0f));
        UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
        Card->SetContent(Content);

        Content->AddChildToVerticalBox(MakeText(
            FString::Printf(TEXT("%s  vs  %s"), *FindTeamName(Rivalry.TeamAId), *FindTeamName(Rivalry.TeamBId)),
            14, DashboardStyle::Primary, true));

        FString IntensityLabel;
        FLinearColor IntensityColor;
        if (Rivalry.Intensity >= 75) { IntensityLabel = TEXT("HEATED"); IntensityColor = FLinearColor(0.95f, 0.32f, 0.28f, 1.0f); }
        else if (Rivalry.Intensity >= 50) { IntensityLabel = TEXT("INTENSE"); IntensityColor = DashboardStyle::Accent; }
        else if (Rivalry.Intensity >= 25) { IntensityLabel = TEXT("GROWING"); IntensityColor = DashboardStyle::Success; }
        else { IntensityLabel = TEXT("BUDDING"); IntensityColor = DashboardStyle::Secondary; }

        UHorizontalBox* StatsRow = WidgetTree->ConstructWidget<UHorizontalBox>();
        StatsRow->AddChildToHorizontalBox(MakeText(IntensityLabel, 11, IntensityColor, true))
            ->SetSize(DashboardStyle::Size(0.25f));
        StatsRow->AddChildToHorizontalBox(MakeText(
            FString::Printf(TEXT("Intensity: %d"), Rivalry.Intensity), 10, DashboardStyle::Secondary))
            ->SetSize(DashboardStyle::Size(0.25f));
        StatsRow->AddChildToHorizontalBox(MakeText(
            FString::Printf(TEXT("Playoff meetings: %d"), Rivalry.PlayoffMeetings), 10, DashboardStyle::Secondary))
            ->SetSize(DashboardStyle::Size(0.25f));
        StatsRow->AddChildToHorizontalBox(MakeText(
            FString::Printf(TEXT("Close games: %d"), Rivalry.CloseGames), 10, DashboardStyle::Secondary))
            ->SetSize(DashboardStyle::Size(0.25f));
        Content->AddChildToVerticalBox(StatsRow)->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));

        if (bInvolvesPlayer)
        {
            const int32 MoraleBonus = FRivalryService::GetMoraleBonus(League, Club.TeamId,
                Rivalry.TeamAId == Club.TeamId ? Rivalry.TeamBId : Rivalry.TeamAId);
            if (MoraleBonus > 0)
            {
                Content->AddChildToVerticalBox(MakeText(
                    FString::Printf(TEXT("+%d morale boost when facing this rival"), MoraleBonus),
                    9, DashboardStyle::Success))->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
            }
        }

        RivalriesList->AddChildToVerticalBox(Card)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
    }
}

void UManagementDashboardWidget::RefreshMatchCenterScreen(const FLeagueState& League, const FTeamState& Club)
{
    MatchCenterList->ClearChildren();

    if (!PendingMatchGameId.IsValid())
    {
        MatchCenterList->AddChildToVerticalBox(MakeText(
            TEXT("No upcoming game. Simulate a round to proceed."), 12, DashboardStyle::Secondary));
        return;
    }

    const FScheduledGame* Game = League.Schedule.FindByPredicate(
        [this](const FScheduledGame& G) { return G.GameId == PendingMatchGameId; });
    if (!Game) { return; }

    const FTeamState* HomeTeam = League.Teams.FindByPredicate(
        [&Game](const FTeamState& T) { return T.TeamId == Game->HomeTeamId; });
    const FTeamState* AwayTeam = League.Teams.FindByPredicate(
        [&Game](const FTeamState& T) { return T.TeamId == Game->AwayTeamId; });
    if (!HomeTeam || !AwayTeam) { return; }

    FTeamPresentationData HomeData = FMatchPresentationService::BuildTeamData(League, *HomeTeam, AwayTeam->TeamId);
    FTeamPresentationData AwayData = FMatchPresentationService::BuildTeamData(League, *AwayTeam, HomeTeam->TeamId);

    MatchCenterList->AddChildToVerticalBox(MakeText(TEXT("MATCH CENTER"), 18, DashboardStyle::Accent, true))
        ->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));

    UHorizontalBox* VsRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    VsRow->AddChildToHorizontalBox(MakeText(
        FString::Printf(TEXT("%s (%d-%d)"), *HomeData.FullName, HomeData.Wins, HomeData.Losses),
        16, DashboardStyle::Primary, true))->SetSize(DashboardStyle::Size(0.4f));
    VsRow->AddChildToHorizontalBox(MakeText(TEXT("VS"), 16, DashboardStyle::Accent, true))
        ->SetSize(DashboardStyle::Size(0.2f));
    VsRow->AddChildToHorizontalBox(MakeText(
        FString::Printf(TEXT("%s (%d-%d)"), *AwayData.FullName, AwayData.Wins, AwayData.Losses),
        16, DashboardStyle::Primary, true))->SetSize(DashboardStyle::Size(0.4f));
    MatchCenterList->AddChildToVerticalBox(VsRow)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));

    FString Implication = FMatchPresentationService::ComputeStandingsImplication(League, Game->HomeTeamId, Game->AwayTeamId);
    if (!Implication.IsEmpty())
    {
        MatchCenterList->AddChildToVerticalBox(MakeText(Implication, 11, FLinearColor(0.95f, 0.80f, 0.25f, 1.0f)))
            ->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
    }

    if (HomeData.RivalryIntensity > 0)
    {
        MatchCenterList->AddChildToVerticalBox(MakeText(
            FString::Printf(TEXT("Rivalry intensity: %d"), HomeData.RivalryIntensity),
            11, FLinearColor(0.95f, 0.32f, 0.28f, 1.0f)))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
    }

    auto AddTeamCard = [&](const FTeamPresentationData& Data, const FString& Label)
    {
        UBorder* Card = MakeCard(DashboardStyle::CardRaised);
        Card->SetPadding(FMargin(12.0f, 10.0f));
        UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
        Card->SetContent(Content);

        Content->AddChildToVerticalBox(MakeText(Label, 12, DashboardStyle::Accent, true));

        if (Data.WinStreak > 0)
        {
            Content->AddChildToVerticalBox(MakeText(
                FString::Printf(TEXT("Win streak: %d  |  Chemistry: %d"), Data.WinStreak, Data.Chemistry),
                10, DashboardStyle::Secondary))->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
        }
        else
        {
            Content->AddChildToVerticalBox(MakeText(
                FString::Printf(TEXT("Chemistry: %d"), Data.Chemistry),
                10, DashboardStyle::Secondary))->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
        }

        FString PaceStr = Data.Tactics.Pace == EPaceStyle::Fast ? TEXT("Fast") :
            Data.Tactics.Pace == EPaceStyle::Slow ? TEXT("Slow") : TEXT("Balanced");
        FString OffStr = Data.Tactics.Offense == EOffenseStyle::Perimeter ? TEXT("Perimeter") :
            Data.Tactics.Offense == EOffenseStyle::Inside ? TEXT("Inside") : TEXT("Balanced");
        FString DefStr = Data.Tactics.Defense == EDefenseStyle::Zone ? TEXT("Zone") :
            Data.Tactics.Defense == EDefenseStyle::Switching ? TEXT("Switching") : TEXT("Man");
        Content->AddChildToVerticalBox(MakeText(
            FString::Printf(TEXT("Tactics: %s / %s / %s"), *PaceStr, *OffStr, *DefStr),
            10, DashboardStyle::Secondary))->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));

        Content->AddChildToVerticalBox(MakeText(TEXT("STARTING FIVE"), 10, DashboardStyle::Accent, true))
            ->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 4.0f));
        for (const FPlayerProfile& Player : Data.StartingFive)
        {
            Content->AddChildToVerticalBox(MakeText(
                FString::Printf(TEXT("%s  —  OVR %d"), *Player.DisplayName, Player.Ratings.Overall()),
                10, DashboardStyle::Primary))->SetPadding(FMargin(0.0f, 1.0f, 0.0f, 0.0f));
        }

        if (Data.InjuredPlayers.Num() > 0)
        {
            Content->AddChildToVerticalBox(MakeText(TEXT("INJURED"), 10,
                FLinearColor(0.95f, 0.32f, 0.28f, 1.0f), true))->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 4.0f));
            for (const FString& Inj : Data.InjuredPlayers)
            {
                Content->AddChildToVerticalBox(MakeText(Inj, 9, DashboardStyle::Secondary));
            }
        }

        MatchCenterList->AddChildToVerticalBox(Card)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
    };

    AddTeamCard(HomeData, FString::Printf(TEXT("HOME — %s"), *HomeData.FullName));
    AddTeamCard(AwayData, FString::Printf(TEXT("AWAY — %s"), *AwayData.FullName));

    UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>();

    UButton* WatchBtn = WidgetTree->ConstructWidget<UButton>();
    WatchBtn->SetBackgroundColor(DashboardStyle::Accent);
    UTextBlock* WatchLabel = MakeText(TEXT("WATCH HIGHLIGHTS"), 12, FLinearColor::White, true);
    WatchBtn->AddChild(WatchLabel);
    WatchBtn->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleWatchHighlights);
    ButtonRow->AddChildToHorizontalBox(WatchBtn)->SetSize(DashboardStyle::Size(0.45f));

    UButton* InstantBtn = WidgetTree->ConstructWidget<UButton>();
    InstantBtn->SetBackgroundColor(DashboardStyle::Card);
    UTextBlock* InstantLabel = MakeText(TEXT("INSTANT RESULT"), 12, DashboardStyle::Accent, true);
    InstantBtn->AddChild(InstantLabel);
    InstantBtn->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleInstantResult);
    ButtonRow->AddChildToHorizontalBox(InstantBtn)->SetSize(DashboardStyle::Size(0.45f));

    MatchCenterList->AddChildToVerticalBox(ButtonRow)->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
}

void UManagementDashboardWidget::HandleWatchHighlights()
{
    ULeagueGameSubsystem* LeagueSubsystem = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>();
    if (!LeagueSubsystem || !PendingMatchGameId.IsValid()) { return; }

    ExecuteSimulateRound();

    if (LastPresentation.GameId.IsValid())
    {
        if (AUnderdogPlayerController* Controller = Cast<AUnderdogPlayerController>(GetOwningPlayer()))
        {
            if (Controller->StartBroadcast(LastPresentation))
            {
                return;
            }
        }
        ReturnFromBroadcast();
    }
}

void UManagementDashboardWidget::ReturnFromBroadcast()
{
    RefreshDashboard();
    SetScreen(18);
    StatusText->SetText(FText::FromString(TEXT("Post-game highlights and recap")));
    StatusText->SetColorAndOpacity(FSlateColor(DashboardStyle::Success));
}

void UManagementDashboardWidget::HandleInstantResult()
{
    if (!PendingMatchGameId.IsValid()) { return; }
    ExecuteSimulateRound();
}

void UManagementDashboardWidget::RefreshPostGameScreen()
{
    PostGameList->ClearChildren();

    if (!LastPresentation.GameId.IsValid())
    {
        PostGameList->AddChildToVerticalBox(MakeText(
            TEXT("No game data. Watch highlights from the Match Center."), 12, DashboardStyle::Secondary));
        return;
    }

    const FMatchPresentationPackage& Pkg = LastPresentation;

    PostGameList->AddChildToVerticalBox(MakeText(TEXT("POST-GAME RECAP"), 18, DashboardStyle::Accent, true))
        ->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    PostGameList->AddChildToVerticalBox(MakeText(
        FString::Printf(TEXT("%s  vs  %s"), *Pkg.Home.FullName, *Pkg.Away.FullName),
        14, DashboardStyle::Primary, true));

    PostGameList->AddChildToVerticalBox(MakeText(
        FString::Printf(TEXT("FINAL: %d - %d"), Pkg.Result.HomeScore, Pkg.Result.AwayScore),
        20, DashboardStyle::Accent, true))->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 8.0f));

    if (Pkg.Recap.QuarterScores.Num() > 0)
    {
        UBorder* QCard = MakeCard(DashboardStyle::Card);
        QCard->SetPadding(FMargin(12.0f, 8.0f));
        UVerticalBox* QContent = WidgetTree->ConstructWidget<UVerticalBox>();
        QCard->SetContent(QContent);

        QContent->AddChildToVerticalBox(MakeText(TEXT("QUARTER SCORES"), 10, DashboardStyle::Accent, true));

        UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>();
        HeaderRow->AddChildToHorizontalBox(MakeText(TEXT(""), 9, DashboardStyle::Secondary))
            ->SetSize(DashboardStyle::Size(0.35f));
        for (int32 Q = 0; Q < Pkg.Recap.QuarterScores.Num(); ++Q)
        {
            FString QLabel = Q < 4 ? FString::Printf(TEXT("Q%d"), Q + 1) : FString::Printf(TEXT("OT%d"), Q - 3);
            HeaderRow->AddChildToHorizontalBox(MakeText(QLabel, 9, DashboardStyle::Secondary, true))
                ->SetSize(DashboardStyle::Size(0.13f));
        }
        HeaderRow->AddChildToHorizontalBox(MakeText(TEXT("TOT"), 9, DashboardStyle::Accent, true))
            ->SetSize(DashboardStyle::Size(0.13f));
        QContent->AddChildToVerticalBox(HeaderRow)->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 2.0f));

        auto AddScoreLine = [&](const FString& TeamName, bool bHome)
        {
            UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
            Row->AddChildToHorizontalBox(MakeText(TeamName, 10, DashboardStyle::Primary, true))
                ->SetSize(DashboardStyle::Size(0.35f));
            for (int32 Q = 0; Q < Pkg.Recap.QuarterScores.Num(); ++Q)
            {
                const int32 Pts = bHome ? Pkg.Recap.QuarterScores[Q].HomePoints : Pkg.Recap.QuarterScores[Q].AwayPoints;
                Row->AddChildToHorizontalBox(MakeText(FString::FromInt(Pts), 10, DashboardStyle::Primary))
                    ->SetSize(DashboardStyle::Size(0.13f));
            }
            const int32 Total = bHome ? Pkg.Result.HomeScore : Pkg.Result.AwayScore;
            Row->AddChildToHorizontalBox(MakeText(FString::FromInt(Total), 10, DashboardStyle::Accent, true))
                ->SetSize(DashboardStyle::Size(0.13f));
            QContent->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
        };
        AddScoreLine(Pkg.Home.FullName, true);
        AddScoreLine(Pkg.Away.FullName, false);

        PostGameList->AddChildToVerticalBox(QCard)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
    }

    if (Pkg.Highlights.Num() > 0)
    {
        UBorder* HCard = MakeCard(DashboardStyle::CardRaised);
        HCard->SetPadding(FMargin(12.0f, 10.0f));
        UVerticalBox* HContent = WidgetTree->ConstructWidget<UVerticalBox>();
        HCard->SetContent(HContent);

        HContent->AddChildToVerticalBox(MakeText(TEXT("HIGHLIGHTS"), 12, DashboardStyle::Accent, true));

        for (const FHighlightCue& Cue : Pkg.Highlights)
        {
            FString TimeStr = Cue.Period <= 4
                ? FString::Printf(TEXT("Q%d %d:%02d"), Cue.Period, Cue.ClockSeconds / 60, Cue.ClockSeconds % 60)
                : FString::Printf(TEXT("OT%d %d:%02d"), Cue.Period - 4, Cue.ClockSeconds / 60, Cue.ClockSeconds % 60);

            UHorizontalBox* CueRow = WidgetTree->ConstructWidget<UHorizontalBox>();
            CueRow->AddChildToHorizontalBox(MakeText(TimeStr, 9, DashboardStyle::Secondary))
                ->SetSize(DashboardStyle::Size(0.15f));
            CueRow->AddChildToHorizontalBox(MakeText(Cue.Description, 9, DashboardStyle::Primary))
                ->SetSize(DashboardStyle::Size(0.55f));
            CueRow->AddChildToHorizontalBox(MakeText(
                FString::Printf(TEXT("%d-%d"), Cue.HomeScoreAfter, Cue.AwayScoreAfter),
                9, DashboardStyle::Accent))->SetSize(DashboardStyle::Size(0.15f));
            CueRow->AddChildToHorizontalBox(MakeText(Cue.PrimaryPlayerName, 9, DashboardStyle::Secondary))
                ->SetSize(DashboardStyle::Size(0.15f));

            HContent->AddChildToVerticalBox(CueRow)->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
        }

        PostGameList->AddChildToVerticalBox(HCard)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
    }

    if (Pkg.Recap.PlayByPlay.Num() > 0)
    {
        UBorder* BoxCard = MakeCard(DashboardStyle::Card);
        BoxCard->SetPadding(FMargin(12.0f, 10.0f));
        UVerticalBox* BoxContent = WidgetTree->ConstructWidget<UVerticalBox>();
        BoxCard->SetContent(BoxContent);

        BoxContent->AddChildToVerticalBox(MakeText(TEXT("KEY PLAYS"), 10, DashboardStyle::Accent, true));

        int32 Count = 0;
        for (const FPlayByPlayEntry& Entry : Pkg.Recap.PlayByPlay)
        {
            if (!Entry.bHighlight || Count >= 15) { continue; }
            Count++;

            FString TimeStr = Entry.Period <= 4
                ? FString::Printf(TEXT("Q%d %d:%02d"), Entry.Period, Entry.ClockSeconds / 60, Entry.ClockSeconds % 60)
                : FString::Printf(TEXT("OT%d %d:%02d"), Entry.Period - 4, Entry.ClockSeconds / 60, Entry.ClockSeconds % 60);

            UHorizontalBox* PlayRow = WidgetTree->ConstructWidget<UHorizontalBox>();
            PlayRow->AddChildToHorizontalBox(MakeText(TimeStr, 9, DashboardStyle::Secondary))
                ->SetSize(DashboardStyle::Size(0.15f));
            PlayRow->AddChildToHorizontalBox(MakeText(Entry.Description, 9, DashboardStyle::Primary))
                ->SetSize(DashboardStyle::Size(0.65f));
            PlayRow->AddChildToHorizontalBox(MakeText(
                FString::Printf(TEXT("%d-%d"), Entry.HomeScore, Entry.AwayScore), 9, DashboardStyle::Accent))
                ->SetSize(DashboardStyle::Size(0.2f));
            BoxContent->AddChildToVerticalBox(PlayRow)->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
        }

        PostGameList->AddChildToVerticalBox(BoxCard)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
    }

    if (!Pkg.Recap.Headline.IsEmpty())
    {
        PostGameList->AddChildToVerticalBox(MakeText(Pkg.Recap.Headline, 12, DashboardStyle::Primary, true))
            ->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
        PostGameList->AddChildToVerticalBox(MakeText(Pkg.Recap.Summary, 10, DashboardStyle::Secondary));
    }

    UButton* ReturnBtn = WidgetTree->ConstructWidget<UButton>();
    ReturnBtn->SetBackgroundColor(DashboardStyle::Accent);
    UTextBlock* ReturnLabel = MakeText(TEXT("RETURN TO DASHBOARD"), 12, FLinearColor::White, true);
    ReturnBtn->AddChild(ReturnLabel);
    ReturnBtn->OnClicked.AddDynamic(this, &UManagementDashboardWidget::ShowOverview);
    PostGameList->AddChildToVerticalBox(ReturnBtn)->SetPadding(FMargin(0.0f, 16.0f, 0.0f, 0.0f));
}

void UManagementDashboardWidget::RefreshFranchiseScreen(const FLeagueState& League, const FTeamState& Club)
{
    FranchiseList->ClearChildren();
    const FFranchiseState& Franchise = Club.Franchise;

    UBorder* FinanceCard = MakeCard(DashboardStyle::CardRaised);
    FinanceCard->SetPadding(FMargin(14.0f, 10.0f));
    UVerticalBox* Finance = WidgetTree->ConstructWidget<UVerticalBox>();
    FinanceCard->SetContent(Finance);
    Finance->AddChildToVerticalBox(MakeText(TEXT("FRANCHISE FINANCES"), 11, DashboardStyle::Accent, true));
    Finance->AddChildToVerticalBox(MakeText(FString::Printf(
        TEXT("Cash $%.1fM   Revenue $%.1fM   Expenses $%.1fM   Profit $%.1fM"),
        Franchise.Finances.CashMinorUnits / 100000000.0,
        Franchise.Finances.SeasonRevenueMinorUnits / 100000000.0,
        Franchise.Finances.SeasonExpensesMinorUnits / 100000000.0,
        Franchise.Finances.OperatingProfit() / 100000000.0), 11, DashboardStyle::Primary, true))
        ->SetPadding(FMargin(0.0f, 5.0f, 0.0f, 0.0f));
    Finance->AddChildToVerticalBox(MakeText(FString::Printf(
        TEXT("Last home gate: %d attendance / $%.2fM revenue"),
        Franchise.Finances.LastAttendance,
        Franchise.Finances.LastGameRevenueMinorUnits / 100000000.0), 10, DashboardStyle::Secondary));

    UHorizontalBox* Pricing = WidgetTree->ConstructWidget<UHorizontalBox>();
    UButton* PriceDown = WidgetTree->ConstructWidget<UButton>();
    PriceDown->SetBackgroundColor(DashboardStyle::Card);
    PriceDown->SetContent(MakeText(TEXT("- $5"), 10, DashboardStyle::Primary, true));
    PriceDown->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleTicketPriceDown);
    Pricing->AddChildToHorizontalBox(PriceDown)->SetPadding(FMargin(0.0f, 4.0f));
    Pricing->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("TICKET PRICE  $%.2f"),
        Franchise.Finances.TicketPriceMinorUnits / 100.0), 11, DashboardStyle::Accent, true))
        ->SetPadding(FMargin(12.0f, 7.0f));
    UButton* PriceUp = WidgetTree->ConstructWidget<UButton>();
    PriceUp->SetBackgroundColor(DashboardStyle::Card);
    PriceUp->SetContent(MakeText(TEXT("+ $5"), 10, DashboardStyle::Primary, true));
    PriceUp->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleTicketPriceUp);
    Pricing->AddChildToHorizontalBox(PriceUp)->SetPadding(FMargin(0.0f, 4.0f));
    Finance->AddChildToVerticalBox(Pricing)->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
    FranchiseList->AddChildToVerticalBox(FinanceCard)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    UBorder* FanCard = MakeCard(DashboardStyle::Card);
    FanCard->SetPadding(FMargin(14.0f, 10.0f));
    FanCard->SetContent(MakeText(FString::Printf(
        TEXT("FANBASE   Support %d/100   Reputation %d/100   Season tickets %d"),
        Franchise.Fanbase.Support, Franchise.Fanbase.Reputation,
        Franchise.Fanbase.SeasonTicketHolders), 11, DashboardStyle::Primary, true));
    FranchiseList->AddChildToVerticalBox(FanCard)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    FranchiseList->AddChildToVerticalBox(MakeText(TEXT("FACILITIES"), 11, DashboardStyle::Accent, true))
        ->SetPadding(FMargin(0.0f, 5.0f, 0.0f, 6.0f));
    auto AddFacility = [&](EFacilityType Type, const FString& Name, const FString& Effect,
        void (UManagementDashboardWidget::*Handler)())
    {
        const int32 Level = FFranchiseService::GetFacilityLevel(Club, Type);
        const int64 Cost = FFranchiseService::GetFacilityUpgradeCost(Club, Type);
        UBorder* Card = MakeCard(DashboardStyle::Card);
        Card->SetPadding(FMargin(12.0f, 8.0f));
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
        Card->SetContent(Row);
        Row->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("%s  LVL %d/5"), *Name, Level),
            11, DashboardStyle::Primary, true))->SetSize(DashboardStyle::Size(0.28f));
        Row->AddChildToHorizontalBox(MakeText(Effect, 9, DashboardStyle::Secondary))
            ->SetSize(DashboardStyle::Size(0.44f));
        UButton* Upgrade = WidgetTree->ConstructWidget<UButton>();
        Upgrade->SetBackgroundColor(Level < 5 ? DashboardStyle::Accent : DashboardStyle::CardRaised);
        Upgrade->SetContent(MakeText(Level < 5
            ? FString::Printf(TEXT("UPGRADE $%.1fM"), Cost / 100000000.0) : TEXT("MAX LEVEL"),
            9, Level < 5 ? DashboardStyle::Background : DashboardStyle::Secondary, true));
        Upgrade->SetIsEnabled(Level < 5);
        if (Handler == &UManagementDashboardWidget::HandleUpgradeTraining)
            { Upgrade->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleUpgradeTraining); }
        else if (Handler == &UManagementDashboardWidget::HandleUpgradeMedical)
            { Upgrade->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleUpgradeMedical); }
        else if (Handler == &UManagementDashboardWidget::HandleUpgradeScouting)
            { Upgrade->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleUpgradeScouting); }
        else { Upgrade->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleUpgradeArena); }
        Row->AddChildToHorizontalBox(Upgrade)->SetSize(DashboardStyle::Size(0.28f));
        FranchiseList->AddChildToVerticalBox(Card)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
    };
    AddFacility(EFacilityType::TrainingCentre, TEXT("TRAINING CENTRE"), TEXT("Improves development chance"),
        &UManagementDashboardWidget::HandleUpgradeTraining);
    AddFacility(EFacilityType::MedicalCentre, TEXT("MEDICAL CENTRE"), TEXT("Reduces fatigue and injury risk"),
        &UManagementDashboardWidget::HandleUpgradeMedical);
    AddFacility(EFacilityType::ScoutingDepartment, TEXT("SCOUTING DEPARTMENT"), TEXT("Narrows report uncertainty"),
        &UManagementDashboardWidget::HandleUpgradeScouting);
    AddFacility(EFacilityType::ArenaOperations, TEXT("ARENA OPERATIONS"), TEXT("Raises capacity and concessions"),
        &UManagementDashboardWidget::HandleUpgradeArena);

    FranchiseList->AddChildToVerticalBox(MakeText(FString::Printf(TEXT("OWNER CONFIDENCE  %d/100"),
        Franchise.Ownership.Confidence), 12, DashboardStyle::Accent, true))
        ->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 5.0f));
    for (const FOwnerObjective& Objective : Franchise.Ownership.Objectives)
    {
        FString Label;
        switch (Objective.Type)
        {
        case EOwnerObjectiveType::WinGames: Label = TEXT("Win games"); break;
        case EOwnerObjectiveType::ReachPlayoffs: Label = TEXT("Reach playoffs"); break;
        case EOwnerObjectiveType::PositiveBalance: Label = TEXT("Maintain positive operating profit"); break;
        default: Label = TEXT("Build team chemistry"); break;
        }
        FranchiseList->AddChildToVerticalBox(MakeText(FString::Printf(TEXT("%s  %d/%d  %s"),
            *Label, Objective.Current, Objective.Target, Objective.bCompleted ? TEXT("COMPLETE") : TEXT("IN PROGRESS")),
            10, Objective.bCompleted ? DashboardStyle::Success : DashboardStyle::Secondary));
    }
}

void UManagementDashboardWidget::HandleTicketPriceDown()
{
    ULeagueGameSubsystem* Subsystem = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>();
    if (!Subsystem) { return; }
    const FLeagueState League = Subsystem->GetLeague();
    if (League.Teams.IsEmpty()) { return; }
    FString Error;
    if (Subsystem->SetTicketPrice(League.Teams[0].TeamId,
        League.Teams[0].Franchise.Finances.TicketPriceMinorUnits - 500, Error))
    { RefreshDashboard(); SetScreen(19); }
    else { StatusText->SetText(FText::FromString(Error)); }
}

void UManagementDashboardWidget::HandleTicketPriceUp()
{
    ULeagueGameSubsystem* Subsystem = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>();
    if (!Subsystem) { return; }
    const FLeagueState League = Subsystem->GetLeague();
    if (League.Teams.IsEmpty()) { return; }
    FString Error;
    if (Subsystem->SetTicketPrice(League.Teams[0].TeamId,
        League.Teams[0].Franchise.Finances.TicketPriceMinorUnits + 500, Error))
    { RefreshDashboard(); SetScreen(19); }
    else { StatusText->SetText(FText::FromString(Error)); }
}

void UManagementDashboardWidget::UpgradeFranchiseFacility(EFacilityType Type)
{
    ULeagueGameSubsystem* Subsystem = GetGameInstance()
        ? GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>() : nullptr;
    if (!Subsystem) { return; }
    const FLeagueState League = Subsystem->GetLeague();
    if (League.Teams.IsEmpty()) { return; }
    FString Error;
    if (Subsystem->UpgradeFacility(League.Teams[0].TeamId, Type, Error))
    { RefreshDashboard(); SetScreen(19); }
    else { StatusText->SetText(FText::FromString(Error)); }
}

void UManagementDashboardWidget::HandleUpgradeTraining()
{
    UpgradeFranchiseFacility(EFacilityType::TrainingCentre);
}
void UManagementDashboardWidget::HandleUpgradeMedical()
{
    UpgradeFranchiseFacility(EFacilityType::MedicalCentre);
}
void UManagementDashboardWidget::HandleUpgradeScouting()
{
    UpgradeFranchiseFacility(EFacilityType::ScoutingDepartment);
}
void UManagementDashboardWidget::HandleUpgradeArena()
{
    UpgradeFranchiseFacility(EFacilityType::ArenaOperations);
}

void UManagementDashboardWidget::RefreshStaffScreen(const FLeagueState& League, const FTeamState& Club)
{
    StaffList->ClearChildren();
    auto RoleName = [](EStaffRole Role)
    {
        switch (Role)
        {
        case EStaffRole::HeadCoach: return FString(TEXT("HEAD COACH"));
        case EStaffRole::OffensiveCoach: return FString(TEXT("OFFENSIVE COACH"));
        case EStaffRole::DefensiveCoach: return FString(TEXT("DEFENSIVE COACH"));
        case EStaffRole::DevelopmentCoach: return FString(TEXT("DEVELOPMENT COACH"));
        case EStaffRole::HeadScout: return FString(TEXT("HEAD SCOUT"));
        default: return FString(TEXT("MEDICAL DIRECTOR"));
        }
    };

    UBorder* Header = MakeCard(DashboardStyle::CardRaised);
    Header->SetPadding(FMargin(16.0f, 11.0f));
    Header->SetContent(MakeText(FString::Printf(
        TEXT("STAFF CHEMISTRY %d   TACTICAL FAMILIARITY %d   PAYROLL $%.1fM"),
        Club.Organization.StaffChemistry, Club.Organization.TacticalFamiliarity,
        Club.Organization.AnnualPayrollMinorUnits / 100000000.0), 12, DashboardStyle::Primary, true));
    StaffList->AddChildToVerticalBox(Header)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));

    StaffList->AddChildToVerticalBox(MakeText(TEXT("YOUR ORGANIZATION"), 10, DashboardStyle::Accent, true))
        ->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 6.0f));
    for (const FStaffMember& Staff : Club.Organization.Staff)
    {
        UBorder* Card = MakeCard(DashboardStyle::Card);
        Card->SetPadding(FMargin(12.0f, 7.0f));
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
        Card->SetContent(Row);
        Row->AddChildToHorizontalBox(MakeText(RoleName(Staff.Role), 9, DashboardStyle::Accent, true))
            ->SetSize(DashboardStyle::Size(0.24f));
        Row->AddChildToHorizontalBox(MakeText(Staff.DisplayName, 11, DashboardStyle::Primary, true))
            ->SetSize(DashboardStyle::Size(0.28f));
        Row->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("OVR %d   REP %d"),
            Staff.Ratings.OverallForRole(Staff.Role), Staff.Reputation), 10, DashboardStyle::Primary))
            ->SetSize(DashboardStyle::Size(0.2f));
        UTextBlock* Contract = MakeText(FString::Printf(TEXT("$%.1fM / %dY"),
            Staff.Contract.SalaryMinorUnits / 100000000.0, Staff.Contract.YearsRemaining),
            10, DashboardStyle::Secondary);
        Contract->SetJustification(ETextJustify::Right);
        Row->AddChildToHorizontalBox(Contract)->SetSize(DashboardStyle::Size(0.2f));
        StaffList->AddChildToVerticalBox(Card)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 3.0f));
    }

    StaffList->AddChildToVerticalBox(MakeText(TEXT("AVAILABLE STAFF"), 10, DashboardStyle::Accent, true))
        ->SetPadding(FMargin(0.0f, 14.0f, 0.0f, 6.0f));
    DisplayedStaffCandidateIds.Reset();
    for (int32 Index = 0; Index < League.StaffMarket.Num() && Index < 6; ++Index)
    {
        const FStaffMember& Candidate = League.StaffMarket[Index];
        DisplayedStaffCandidateIds.Add(Candidate.StaffId);
        UBorder* Card = MakeCard(DashboardStyle::CardRaised);
        Card->SetPadding(FMargin(12.0f, 7.0f));
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
        Card->SetContent(Row);
        Row->AddChildToHorizontalBox(MakeText(RoleName(Candidate.Role), 9, DashboardStyle::Secondary, true))
            ->SetSize(DashboardStyle::Size(0.22f));
        Row->AddChildToHorizontalBox(MakeText(Candidate.DisplayName, 11, DashboardStyle::Primary, true))
            ->SetSize(DashboardStyle::Size(0.25f));
        Row->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("OVR %d   AGE %d"),
            Candidate.Ratings.OverallForRole(Candidate.Role), Candidate.Age), 10, DashboardStyle::Primary))
            ->SetSize(DashboardStyle::Size(0.2f));
        UButton* Hire = WidgetTree->ConstructWidget<UButton>();
        Hire->SetBackgroundColor(DashboardStyle::Accent);
        Hire->SetContent(MakeText(FString::Printf(TEXT("HIRE  $%.1fM / 2Y"),
            Candidate.Contract.SalaryMinorUnits / 100000000.0), 9, DashboardStyle::Primary, true));
        BindStaffHireButton(Hire, Index);
        Row->AddChildToHorizontalBox(Hire)->SetSize(DashboardStyle::Size(0.24f));
        StaffList->AddChildToVerticalBox(Card)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 3.0f));
    }
}

void UManagementDashboardWidget::BindStaffHireButton(UButton* Button, int32 Index)
{
    switch (Index)
    {
    case 0: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleHireStaff0); break;
    case 1: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleHireStaff1); break;
    case 2: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleHireStaff2); break;
    case 3: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleHireStaff3); break;
    case 4: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleHireStaff4); break;
    case 5: Button->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleHireStaff5); break;
    default: break;
    }
}

void UManagementDashboardWidget::HireStaffAtSlot(int32 Index)
{
    if (!DisplayedStaffCandidateIds.IsValidIndex(Index)) { return; }
    ULeagueGameSubsystem* Subsystem = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>();
    if (!Subsystem) { return; }
    const FLeagueState League = Subsystem->GetLeague();
    if (League.Teams.IsEmpty()) { return; }
    const FStaffMember* Candidate = League.StaffMarket.FindByPredicate(
        [this, Index](const FStaffMember& Staff) { return Staff.StaffId == DisplayedStaffCandidateIds[Index]; });
    if (!Candidate) { return; }
    FString Error;
    if (Subsystem->HireStaff(League.Teams[0].TeamId, Candidate->StaffId,
        Candidate->Contract.SalaryMinorUnits, 2, Error))
    {
        RefreshDashboard();
        SetScreen(20);
        StatusText->SetText(FText::FromString(TEXT("Staff appointment completed.")));
    }
    else { StatusText->SetText(FText::FromString(Error)); }
}

void UManagementDashboardWidget::HandleHireStaff0() { HireStaffAtSlot(0); }
void UManagementDashboardWidget::HandleHireStaff1() { HireStaffAtSlot(1); }
void UManagementDashboardWidget::HandleHireStaff2() { HireStaffAtSlot(2); }
void UManagementDashboardWidget::HandleHireStaff3() { HireStaffAtSlot(3); }
void UManagementDashboardWidget::HandleHireStaff4() { HireStaffAtSlot(4); }
void UManagementDashboardWidget::HandleHireStaff5() { HireStaffAtSlot(5); }

void UManagementDashboardWidget::RefreshCareerScreen(const FLeagueState& League)
{
    CareerList->ClearChildren();
    const FManagerCareer& Career = League.ManagerCareer;
    const FTeamState* CurrentTeam = League.Teams.FindByPredicate(
        [&Career](const FTeamState& Team) { return Team.TeamId == Career.CurrentTeamId; });
    const FString Employment = Career.EmploymentStatus == EManagerEmploymentStatus::Employed
        ? FString::Printf(TEXT("%s  |  %dY CONTRACT"),
            CurrentTeam ? *(CurrentTeam->City + TEXT(" ") + CurrentTeam->Nickname) : TEXT("UNKNOWN CLUB"),
            Career.ContractYearsRemaining)
        : TEXT("UNEMPLOYED - REVIEW AVAILABLE OFFERS");

    UBorder* Hero = MakeCard(DashboardStyle::CardRaised);
    Hero->SetPadding(FMargin(18.0f, 14.0f));
    UVerticalBox* HeroContent = WidgetTree->ConstructWidget<UVerticalBox>();
    Hero->SetContent(HeroContent);
    HeroContent->AddChildToVerticalBox(MakeText(Career.ManagerName.ToUpper(), 24, DashboardStyle::Primary, true));
    HeroContent->AddChildToVerticalBox(MakeText(Employment, 10, DashboardStyle::Accent, true));
    HeroContent->AddChildToVerticalBox(MakeText(FString::Printf(
        TEXT("CAREER %d-%d   |   PLAYOFFS %d   |   TITLES %d   |   LEGACY SCORE %d"),
        Career.CareerWins, Career.CareerLosses, Career.PlayoffAppearances,
        Career.Championships, Career.CareerScore), 12, DashboardStyle::Primary, true))
        ->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
    CareerList->AddChildToVerticalBox(Hero)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));

    CareerList->AddChildToVerticalBox(MakeText(TEXT("SEASON HISTORY"), 10, DashboardStyle::Accent, true));
    if (Career.SeasonHistory.IsEmpty())
    {
        CareerList->AddChildToVerticalBox(MakeText(
            TEXT("Complete your first season to establish a managerial record."), 10, DashboardStyle::Secondary));
    }
    for (int32 Index = Career.SeasonHistory.Num() - 1; Index >= 0; --Index)
    {
        const FManagerSeasonRecord& Record = Career.SeasonHistory[Index];
        CareerList->AddChildToVerticalBox(MakeText(FString::Printf(
            TEXT("S%d  |  %s  |  %d-%d  |  %s  |  OWNER %d"), Record.Season,
            *Record.TeamName, Record.Wins, Record.Losses,
            Record.bChampion ? TEXT("CHAMPION") : Record.bReachedPlayoffs ? TEXT("PLAYOFFS") : TEXT("LOTTERY"),
            Record.OwnerConfidence), 10,
            Record.bChampion ? DashboardStyle::Success : DashboardStyle::Primary, Record.bChampion));
    }

    DisplayedJobOfferTeamIds.Reset();
    if (!Career.JobOffers.IsEmpty())
    {
        CareerList->AddChildToVerticalBox(MakeText(TEXT("JOB MARKET"), 10, DashboardStyle::Accent, true))
            ->SetPadding(FMargin(0.0f, 14.0f, 0.0f, 6.0f));
    }
    for (int32 Index = 0; Index < Career.JobOffers.Num() && Index < 3; ++Index)
    {
        const FManagerJobOffer& Offer = Career.JobOffers[Index];
        DisplayedJobOfferTeamIds.Add(Offer.TeamId);
        UBorder* Card = MakeCard(DashboardStyle::Card);
        Card->SetPadding(FMargin(12.0f, 8.0f));
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
        Card->SetContent(Row);
        Row->AddChildToHorizontalBox(MakeText(Offer.TeamName, 12, DashboardStyle::Primary, true))
            ->SetSize(DashboardStyle::Size(0.5f));
        Row->AddChildToHorizontalBox(MakeText(FString::Printf(TEXT("%dY  |  TARGET %d WINS"),
            Offer.ContractYears, Offer.ExpectedWins), 10, DashboardStyle::Secondary))
            ->SetSize(DashboardStyle::Size(0.3f));
        UButton* Accept = WidgetTree->ConstructWidget<UButton>();
        Accept->SetBackgroundColor(DashboardStyle::Accent);
        Accept->SetContent(MakeText(TEXT("ACCEPT JOB"), 9, DashboardStyle::Primary, true));
        if (Index == 0) { Accept->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleAcceptJob0); }
        else if (Index == 1) { Accept->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleAcceptJob1); }
        else { Accept->OnClicked.AddDynamic(this, &UManagementDashboardWidget::HandleAcceptJob2); }
        Row->AddChildToHorizontalBox(Accept)->SetSize(DashboardStyle::Size(0.2f));
        CareerList->AddChildToVerticalBox(Card)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
    }
}

void UManagementDashboardWidget::AcceptJobAtSlot(int32 Index)
{
    if (!DisplayedJobOfferTeamIds.IsValidIndex(Index)) { return; }
    ULeagueGameSubsystem* Subsystem = GetGameInstance()->GetSubsystem<ULeagueGameSubsystem>();
    if (!Subsystem) { return; }
    FString Error;
    if (Subsystem->AcceptManagerJob(DisplayedJobOfferTeamIds[Index], Error))
    {
        SelectedTradeTeamId.Invalidate();
        TradeOutgoingPlayerIds.Reset();
        TradeIncomingPlayerIds.Reset();
        RefreshDashboard();
        SetScreen(21);
        StatusText->SetText(FText::FromString(TEXT("New club appointment accepted.")));
    }
    else { StatusText->SetText(FText::FromString(Error)); }
}

void UManagementDashboardWidget::HandleAcceptJob0() { AcceptJobAtSlot(0); }
void UManagementDashboardWidget::HandleAcceptJob1() { AcceptJobAtSlot(1); }
void UManagementDashboardWidget::HandleAcceptJob2() { AcceptJobAtSlot(2); }
