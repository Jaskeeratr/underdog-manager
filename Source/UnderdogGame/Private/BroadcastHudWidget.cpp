#include "BroadcastHudWidget.h"

#include "BroadcastArenaDirector.h"
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

namespace BroadcastHudStyle
{
    const FLinearColor Dark(0.015f, 0.025f, 0.055f, 0.92f);
    const FLinearColor Light(0.93f, 0.96f, 1.0f, 1.0f);
    const FLinearColor Accent(0.98f, 0.45f, 0.08f, 1.0f);
}

static UTextBlock* MakeHudText(UWidgetTree* Tree, const FString& Value, int32 Size, bool bBold = false)
{
    UTextBlock* Text = Tree->ConstructWidget<UTextBlock>();
    Text->SetText(FText::FromString(Value));
    Text->SetColorAndOpacity(FSlateColor(bBold ? BroadcastHudStyle::Accent : BroadcastHudStyle::Light));
    FSlateFontInfo Font = Text->GetFont();
    Font.Size = Size;
    Text->SetFont(Font);
    return Text;
}

void UBroadcastHudWidget::InitializeForDirector(ABroadcastArenaDirector* InDirector)
{
    Director = InDirector;
    RefreshLabels();
}

TSharedRef<SWidget> UBroadcastHudWidget::RebuildWidget()
{
    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
    WidgetTree->RootWidget = Root;

    UBorder* Scoreboard = WidgetTree->ConstructWidget<UBorder>();
    Scoreboard->SetBrushColor(BroadcastHudStyle::Dark);
    Scoreboard->SetPadding(FMargin(18.0f, 10.0f));
    UVerticalBox* ScoreStack = WidgetTree->ConstructWidget<UVerticalBox>();
    Scoreboard->SetContent(ScoreStack);

    MatchupText = MakeHudText(WidgetTree, TEXT("UNDERDOG BROADCAST"), 12, true);
    ScoreText = MakeHudText(WidgetTree, TEXT("0 - 0"), 24, true);
    ClockText = MakeHudText(WidgetTree, TEXT("Q1 12:00"), 11);
    ScoreStack->AddChildToVerticalBox(MatchupText);
    ScoreStack->AddChildToVerticalBox(ScoreText);
    ScoreStack->AddChildToVerticalBox(ClockText);

    UOverlaySlot* ScoreSlot = Root->AddChildToOverlay(Scoreboard);
    ScoreSlot->SetHorizontalAlignment(HAlign_Center);
    ScoreSlot->SetVerticalAlignment(VAlign_Top);
    ScoreSlot->SetPadding(FMargin(0.0f, 24.0f, 0.0f, 0.0f));

    UBorder* LowerThird = WidgetTree->ConstructWidget<UBorder>();
    LowerThird->SetBrushColor(BroadcastHudStyle::Dark);
    LowerThird->SetPadding(FMargin(18.0f, 10.0f));
    UVerticalBox* ControlStack = WidgetTree->ConstructWidget<UVerticalBox>();
    LowerThird->SetContent(ControlStack);
    CueText = MakeHudText(WidgetTree, TEXT("Preparing highlight..."), 12);
    ControlStack->AddChildToVerticalBox(CueText)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    UHorizontalBox* Controls = WidgetTree->ConstructWidget<UHorizontalBox>();
    auto AddButton = [&](const FString& Label, FName Name, void (UBroadcastHudWidget::*Handler)()) -> UTextBlock*
    {
        UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
        Button->SetBackgroundColor(FLinearColor(0.12f, 0.18f, 0.3f, 1.0f));
        UTextBlock* ButtonLabel = MakeHudText(WidgetTree, Label, 10);
        Button->SetContent(ButtonLabel);
        if (Handler == &UBroadcastHudWidget::HandlePause) { Button->OnClicked.AddDynamic(this, &UBroadcastHudWidget::HandlePause); }
        else if (Handler == &UBroadcastHudWidget::HandleSkip) { Button->OnClicked.AddDynamic(this, &UBroadcastHudWidget::HandleSkip); }
        else if (Handler == &UBroadcastHudWidget::HandleSpeed) { Button->OnClicked.AddDynamic(this, &UBroadcastHudWidget::HandleSpeed); }
        else { Button->OnClicked.AddDynamic(this, &UBroadcastHudWidget::HandleExit); }
        Controls->AddChildToHorizontalBox(Button)->SetPadding(FMargin(4.0f));
        return ButtonLabel;
    };

    PauseText = AddButton(TEXT("PAUSE"), TEXT("PauseButton"), &UBroadcastHudWidget::HandlePause);
    AddButton(TEXT("SKIP PLAY"), TEXT("SkipButton"), &UBroadcastHudWidget::HandleSkip);
    SpeedText = AddButton(TEXT("1x SPEED"), TEXT("SpeedButton"), &UBroadcastHudWidget::HandleSpeed);
    AddButton(TEXT("EXIT TO RECAP"), TEXT("ExitButton"), &UBroadcastHudWidget::HandleExit);
    ControlStack->AddChildToVerticalBox(Controls);

    UOverlaySlot* LowerSlot = Root->AddChildToOverlay(LowerThird);
    LowerSlot->SetHorizontalAlignment(HAlign_Center);
    LowerSlot->SetVerticalAlignment(VAlign_Bottom);
    LowerSlot->SetPadding(FMargin(80.0f, 0.0f, 80.0f, 28.0f));

    return Super::RebuildWidget();
}

void UBroadcastHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    RefreshLabels();
}

void UBroadcastHudWidget::RefreshLabels()
{
    if (!Director || !MatchupText || !ScoreText || !ClockText || !CueText) { return; }
    const FMatchPresentationPackage& Pkg = Director->GetPresentation();
    MatchupText->SetText(FText::FromString(FString::Printf(
        TEXT("%s  vs  %s"), *Pkg.Home.FullName, *Pkg.Away.FullName)));

    if (const FHighlightCue* Cue = Director->GetCurrentCue())
    {
        ScoreText->SetText(FText::FromString(FString::Printf(
            TEXT("%d  -  %d"), Cue->HomeScoreAfter, Cue->AwayScoreAfter)));
        const FString Period = Cue->Period <= 4
            ? FString::Printf(TEXT("Q%d"), Cue->Period)
            : FString::Printf(TEXT("OT%d"), Cue->Period - 4);
        ClockText->SetText(FText::FromString(FString::Printf(
            TEXT("%s  %d:%02d  |  Highlight %d/%d"), *Period,
            Cue->ClockSeconds / 60, Cue->ClockSeconds % 60,
            Director->GetCueIndex() + 1, Director->GetCueCount())));
        CueText->SetText(FText::FromString(Cue->Description));
    }
    if (PauseText) { PauseText->SetText(FText::FromString(Director->IsPaused() ? TEXT("RESUME") : TEXT("PAUSE"))); }
    if (SpeedText) { SpeedText->SetText(FText::FromString(FString::Printf(TEXT("%.1fx SPEED"), Director->GetPlaybackSpeed()))); }
}

void UBroadcastHudWidget::HandlePause() { if (Director) { Director->TogglePause(); } }
void UBroadcastHudWidget::HandleSkip() { if (Director) { Director->SkipCue(); } }

void UBroadcastHudWidget::HandleSpeed()
{
    if (!Director) { return; }
    const float Current = Director->GetPlaybackSpeed();
    Director->SetPlaybackSpeed(Current < 1.5f ? 2.0f : Current < 3.0f ? 4.0f : 1.0f);
}

void UBroadcastHudWidget::HandleExit() { if (Director) { Director->StopBroadcast(); } }
