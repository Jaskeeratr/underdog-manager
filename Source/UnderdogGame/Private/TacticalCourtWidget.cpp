#include "TacticalCourtWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace CourtStyle
{
    const FLinearColor Floor(0.16f, 0.10f, 0.06f, 1.0f);
    const FLinearColor Lines(0.85f, 0.80f, 0.70f, 0.6f);
    const FLinearColor HomeColor(0.20f, 0.55f, 0.90f, 1.0f);
    const FLinearColor AwayColor(0.90f, 0.25f, 0.20f, 1.0f);
    const FLinearColor BallColor(0.96f, 0.55f, 0.12f, 1.0f);
    const FLinearColor HudBg(0.02f, 0.03f, 0.06f, 0.88f);
    const FLinearColor HudText(0.93f, 0.96f, 1.0f, 1.0f);
    const FLinearColor Accent(0.98f, 0.50f, 0.10f, 1.0f);
    const FLinearColor KeyPaint(0.22f, 0.08f, 0.04f, 0.5f);
    const FLinearColor DimText(0.55f, 0.60f, 0.68f, 1.0f);

    constexpr float CourtAspect = 1.88f;
    constexpr float DotSize = 18.0f;
    constexpr float BallSize = 12.0f;
}

UBorder* UTacticalCourtWidget::MakeDot(float Size, const FLinearColor& Color)
{
    UBorder* Dot = WidgetTree->ConstructWidget<UBorder>();
    Dot->SetBrushColor(Color);
    Dot->SetDesiredSizeScale(FVector2D(Size, Size));
    Dot->SetPadding(FMargin(0.0f));
    return Dot;
}

UBorder* UTacticalCourtWidget::MakeCourtLine(float X, float Y, float W, float H, const FLinearColor& Color)
{
    UBorder* Line = WidgetTree->ConstructWidget<UBorder>();
    Line->SetBrushColor(Color);
    UCanvasPanelSlot* Slot = CourtCanvas->AddChildToCanvas(Line);
    Slot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
    Slot->SetOffsets(FMargin(X, Y, W, H));
    Slot->SetAutoSize(false);
    return Line;
}

UTextBlock* UTacticalCourtWidget::MakeLabel(const FString& Text, int32 Size, const FLinearColor& Color, bool bBold)
{
    UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>();
    Block->SetText(FText::FromString(Text));
    Block->SetColorAndOpacity(FSlateColor(Color));
    FSlateFontInfo Font = Block->GetFont();
    Font.Size = Size;
    Block->SetFont(Font);
    return Block;
}

TSharedRef<SWidget> UTacticalCourtWidget::RebuildWidget()
{
    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
    WidgetTree->RootWidget = Root;

    // ── Full-screen dark background ──
    UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
    Background->SetBrushColor(FLinearColor(0.01f, 0.015f, 0.03f, 1.0f));
    Root->AddChildToOverlay(Background);

    // ── Court area ──
    UBorder* CourtBorder = WidgetTree->ConstructWidget<UBorder>();
    CourtBorder->SetBrushColor(CourtStyle::Floor);
    CourtBorder->SetPadding(FMargin(3.0f));

    CourtCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    CourtBorder->SetContent(CourtCanvas);

    UOverlaySlot* CourtSlot = Root->AddChildToOverlay(CourtBorder);
    CourtSlot->SetHorizontalAlignment(HAlign_Center);
    CourtSlot->SetVerticalAlignment(VAlign_Center);
    CourtSlot->SetPadding(FMargin(40.0f, 80.0f, 40.0f, 100.0f));

    // ── Court markings (positioned in normalized 600x320 space, scaled in tick) ──
    // Center line
    MakeCourtLine(298.0f, 0.0f, 4.0f, 320.0f, CourtStyle::Lines);
    // Left key (paint area)
    MakeCourtLine(0.0f, 80.0f, 114.0f, 160.0f, CourtStyle::KeyPaint);
    MakeCourtLine(114.0f, 80.0f, 3.0f, 160.0f, CourtStyle::Lines);
    MakeCourtLine(0.0f, 80.0f, 3.0f, 160.0f, CourtStyle::Lines);
    MakeCourtLine(0.0f, 78.0f, 114.0f, 3.0f, CourtStyle::Lines);
    MakeCourtLine(0.0f, 237.0f, 114.0f, 3.0f, CourtStyle::Lines);
    // Right key
    MakeCourtLine(486.0f, 80.0f, 114.0f, 160.0f, CourtStyle::KeyPaint);
    MakeCourtLine(483.0f, 80.0f, 3.0f, 160.0f, CourtStyle::Lines);
    MakeCourtLine(597.0f, 80.0f, 3.0f, 160.0f, CourtStyle::Lines);
    MakeCourtLine(486.0f, 78.0f, 114.0f, 3.0f, CourtStyle::Lines);
    MakeCourtLine(486.0f, 237.0f, 114.0f, 3.0f, CourtStyle::Lines);
    // Free throw lines
    MakeCourtLine(110.0f, 158.0f, 6.0f, 6.0f, CourtStyle::Lines);
    MakeCourtLine(484.0f, 158.0f, 6.0f, 6.0f, CourtStyle::Lines);
    // Baskets (small squares representing hoops)
    MakeCourtLine(8.0f, 152.0f, 14.0f, 16.0f, CourtStyle::Accent);
    MakeCourtLine(578.0f, 152.0f, 14.0f, 16.0f, CourtStyle::Accent);
    // Three-point approximation (wing markers)
    MakeCourtLine(130.0f, 24.0f, 4.0f, 10.0f, CourtStyle::Lines);
    MakeCourtLine(130.0f, 286.0f, 4.0f, 10.0f, CourtStyle::Lines);
    MakeCourtLine(466.0f, 24.0f, 4.0f, 10.0f, CourtStyle::Lines);
    MakeCourtLine(466.0f, 286.0f, 4.0f, 10.0f, CourtStyle::Lines);
    // Top of arc markers
    MakeCourtLine(152.0f, 10.0f, 4.0f, 8.0f, CourtStyle::Lines);
    MakeCourtLine(152.0f, 302.0f, 4.0f, 8.0f, CourtStyle::Lines);
    MakeCourtLine(444.0f, 10.0f, 4.0f, 8.0f, CourtStyle::Lines);
    MakeCourtLine(444.0f, 302.0f, 4.0f, 8.0f, CourtStyle::Lines);
    // Center circle dot
    MakeCourtLine(296.0f, 156.0f, 8.0f, 8.0f, CourtStyle::Lines);

    // ── Player dots ──
    HomeBasePositions = {
        {0.08f, 0.30f}, {0.08f, 0.70f}, {0.15f, 0.50f}, {0.20f, 0.22f}, {0.20f, 0.78f}
    };
    AwayBasePositions = {
        {0.92f, 0.30f}, {0.92f, 0.70f}, {0.85f, 0.50f}, {0.80f, 0.22f}, {0.80f, 0.78f}
    };

    for (int32 I = 0; I < 5; ++I)
    {
        UBorder* Dot = MakeDot(CourtStyle::DotSize, CourtStyle::HomeColor);
        UCanvasPanelSlot* DotSlot = CourtCanvas->AddChildToCanvas(Dot);
        DotSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
        DotSlot->SetAutoSize(true);
        HomeDots.Add(Dot);
    }
    for (int32 I = 0; I < 5; ++I)
    {
        UBorder* Dot = MakeDot(CourtStyle::DotSize, CourtStyle::AwayColor);
        UCanvasPanelSlot* DotSlot = CourtCanvas->AddChildToCanvas(Dot);
        DotSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
        DotSlot->SetAutoSize(true);
        AwayDots.Add(Dot);
    }

    BallDot = MakeDot(CourtStyle::BallSize, CourtStyle::BallColor);
    UCanvasPanelSlot* BallSlot = CourtCanvas->AddChildToCanvas(BallDot);
    BallSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
    BallSlot->SetAutoSize(true);

    // ── Scoreboard HUD (top center) ──
    UBorder* Scoreboard = WidgetTree->ConstructWidget<UBorder>();
    Scoreboard->SetBrushColor(CourtStyle::HudBg);
    Scoreboard->SetPadding(FMargin(24.0f, 10.0f));
    UVerticalBox* ScoreStack = WidgetTree->ConstructWidget<UVerticalBox>();
    Scoreboard->SetContent(ScoreStack);

    MatchupText = MakeLabel(TEXT("HOME  vs  AWAY"), 11, CourtStyle::DimText);
    ScoreText = MakeLabel(TEXT("0  -  0"), 26, CourtStyle::Accent, true);
    ClockText = MakeLabel(TEXT("Q1 12:00"), 10, CourtStyle::DimText);
    CueCountText = MakeLabel(TEXT("Highlight 1/10"), 9, CourtStyle::DimText);

    ScoreStack->AddChildToVerticalBox(MatchupText);
    ScoreStack->AddChildToVerticalBox(ScoreText)->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 2.0f));
    ScoreStack->AddChildToVerticalBox(ClockText);
    ScoreStack->AddChildToVerticalBox(CueCountText)->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));

    UOverlaySlot* ScoreSlot = Root->AddChildToOverlay(Scoreboard);
    ScoreSlot->SetHorizontalAlignment(HAlign_Center);
    ScoreSlot->SetVerticalAlignment(VAlign_Top);
    ScoreSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));

    // ── Play description (bottom center) ──
    UBorder* PlayBar = WidgetTree->ConstructWidget<UBorder>();
    PlayBar->SetBrushColor(CourtStyle::HudBg);
    PlayBar->SetPadding(FMargin(20.0f, 8.0f));
    PlayText = MakeLabel(TEXT("Preparing broadcast..."), 12, CourtStyle::HudText);
    PlayBar->SetContent(PlayText);

    UOverlaySlot* PlaySlot = Root->AddChildToOverlay(PlayBar);
    PlaySlot->SetHorizontalAlignment(HAlign_Center);
    PlaySlot->SetVerticalAlignment(VAlign_Bottom);
    PlaySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 48.0f));

    // ── Controls (bottom right) ──
    UBorder* ControlBar = WidgetTree->ConstructWidget<UBorder>();
    ControlBar->SetBrushColor(CourtStyle::HudBg);
    ControlBar->SetPadding(FMargin(10.0f, 6.0f));
    UHorizontalBox* Controls = WidgetTree->ConstructWidget<UHorizontalBox>();
    ControlBar->SetContent(Controls);

    auto AddControlButton = [&](const FString& Label, FName Name,
        void (UTacticalCourtWidget::*Handler)()) -> UTextBlock*
    {
        UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
        Btn->SetBackgroundColor(FLinearColor(0.10f, 0.14f, 0.24f, 1.0f));
        UTextBlock* BtnLabel = MakeLabel(Label, 10, CourtStyle::HudText);
        Btn->SetContent(BtnLabel);
        if (Handler == &UTacticalCourtWidget::HandlePause)
            Btn->OnClicked.AddDynamic(this, &UTacticalCourtWidget::HandlePause);
        else if (Handler == &UTacticalCourtWidget::HandleSkip)
            Btn->OnClicked.AddDynamic(this, &UTacticalCourtWidget::HandleSkip);
        else if (Handler == &UTacticalCourtWidget::HandleSpeed)
            Btn->OnClicked.AddDynamic(this, &UTacticalCourtWidget::HandleSpeed);
        else
            Btn->OnClicked.AddDynamic(this, &UTacticalCourtWidget::HandleExit);
        Controls->AddChildToHorizontalBox(Btn)->SetPadding(FMargin(3.0f));
        return BtnLabel;
    };

    PauseLabel = AddControlButton(TEXT("PAUSE"), TEXT("BtnPause"), &UTacticalCourtWidget::HandlePause);
    AddControlButton(TEXT("SKIP"), TEXT("BtnSkip"), &UTacticalCourtWidget::HandleSkip);
    SpeedLabel = AddControlButton(TEXT("1x"), TEXT("BtnSpeed"), &UTacticalCourtWidget::HandleSpeed);
    AddControlButton(TEXT("EXIT"), TEXT("BtnExit"), &UTacticalCourtWidget::HandleExit);

    UOverlaySlot* CtrlSlot = Root->AddChildToOverlay(ControlBar);
    CtrlSlot->SetHorizontalAlignment(HAlign_Right);
    CtrlSlot->SetVerticalAlignment(VAlign_Bottom);
    CtrlSlot->SetPadding(FMargin(0.0f, 0.0f, 20.0f, 10.0f));

    // ── Team legend (bottom left) ──
    UBorder* Legend = WidgetTree->ConstructWidget<UBorder>();
    Legend->SetBrushColor(CourtStyle::HudBg);
    Legend->SetPadding(FMargin(10.0f, 6.0f));
    UVerticalBox* LegendStack = WidgetTree->ConstructWidget<UVerticalBox>();
    Legend->SetContent(LegendStack);

    UHorizontalBox* HomeLegend = WidgetTree->ConstructWidget<UHorizontalBox>();
    UBorder* HomeSquare = WidgetTree->ConstructWidget<UBorder>();
    HomeSquare->SetBrushColor(CourtStyle::HomeColor);
    HomeSquare->SetDesiredSizeScale(FVector2D(10.0f, 10.0f));
    HomeLegend->AddChildToHorizontalBox(HomeSquare)->SetPadding(FMargin(0.0f, 2.0f, 6.0f, 0.0f));
    HomeLegend->AddChildToHorizontalBox(MakeLabel(TEXT("HOME"), 9, CourtStyle::HomeColor));
    LegendStack->AddChildToVerticalBox(HomeLegend);

    UHorizontalBox* AwayLegend = WidgetTree->ConstructWidget<UHorizontalBox>();
    UBorder* AwaySquare = WidgetTree->ConstructWidget<UBorder>();
    AwaySquare->SetBrushColor(CourtStyle::AwayColor);
    AwaySquare->SetDesiredSizeScale(FVector2D(10.0f, 10.0f));
    AwayLegend->AddChildToHorizontalBox(AwaySquare)->SetPadding(FMargin(0.0f, 2.0f, 6.0f, 0.0f));
    AwayLegend->AddChildToHorizontalBox(MakeLabel(TEXT("AWAY"), 9, CourtStyle::AwayColor));
    LegendStack->AddChildToVerticalBox(AwayLegend)->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));

    UOverlaySlot* LegendSlot = Root->AddChildToOverlay(Legend);
    LegendSlot->SetHorizontalAlignment(HAlign_Left);
    LegendSlot->SetVerticalAlignment(VAlign_Bottom);
    LegendSlot->SetPadding(FMargin(20.0f, 0.0f, 0.0f, 10.0f));

    return Super::RebuildWidget();
}

void UTacticalCourtWidget::InitializeBroadcast(const FMatchPresentationPackage& InPresentation)
{
    Presentation = InPresentation;
    CurrentCueIndex = 0;
    CueElapsed = 0.0f;
    bPaused = false;
    bFinished = false;
    PlaybackSpeed = 1.0f;
    RefreshScoreboard();
    ApplyPositions(0.0f);
}

// ── Choreography per highlight template ──

UTacticalCourtWidget::FCueChoreography UTacticalCourtWidget::GetChoreography(
    EHighlightTemplate Template, bool bHomeTeam)
{
    const float BasketX = bHomeTeam ? 0.95f : 0.05f;
    const float HalfX = 0.50f;
    const float FTX = bHomeTeam ? 0.82f : 0.18f;
    const float WingX = bHomeTeam ? 0.72f : 0.28f;
    const float ArcX = bHomeTeam ? 0.68f : 0.32f;
    const float DeepX = bHomeTeam ? 0.60f : 0.40f;
    const float Dir = bHomeTeam ? 1.0f : -1.0f;

    FCueChoreography C;

    switch (Template)
    {
    case EHighlightTemplate::ThreePointer:
        C.PrimaryStart = {ArcX, 0.25f};
        C.PrimaryEnd = {ArcX, 0.30f};
        C.SecondaryStart = {WingX, 0.70f};
        C.SecondaryEnd = {WingX, 0.65f};
        C.BallStart = {ArcX, 0.25f};
        C.BallMid = {(ArcX + BasketX) * 0.5f, 0.15f};
        C.BallEnd = {BasketX, 0.50f};
        break;

    case EHighlightTemplate::DriveAndFinish:
        C.PrimaryStart = {WingX, 0.35f};
        C.PrimaryEnd = {BasketX - Dir * 0.03f, 0.48f};
        C.SecondaryStart = {WingX, 0.65f};
        C.SecondaryEnd = {WingX - Dir * 0.02f, 0.62f};
        C.BallStart = {WingX, 0.35f};
        C.BallMid = {(WingX + BasketX) * 0.5f, 0.42f};
        C.BallEnd = {BasketX, 0.50f};
        break;

    case EHighlightTemplate::AssistedBasket:
        C.PrimaryStart = {FTX, 0.60f};
        C.PrimaryEnd = {BasketX - Dir * 0.04f, 0.50f};
        C.SecondaryStart = {DeepX, 0.30f};
        C.SecondaryEnd = {WingX, 0.35f};
        C.BallStart = {DeepX, 0.30f};
        C.BallMid = {FTX, 0.45f};
        C.BallEnd = {BasketX, 0.50f};
        break;

    case EHighlightTemplate::BlockPlay:
        C.PrimaryStart = {WingX, 0.40f};
        C.PrimaryEnd = {FTX, 0.48f};
        C.SecondaryStart = {FTX + Dir * 0.03f, 0.52f};
        C.SecondaryEnd = {FTX, 0.48f};
        C.BallStart = {WingX, 0.40f};
        C.BallMid = {FTX, 0.30f};
        C.BallEnd = {HalfX, 0.50f};
        break;

    case EHighlightTemplate::StealFastBreak:
        C.PrimaryStart = {HalfX, 0.45f};
        C.PrimaryEnd = {BasketX - Dir * 0.03f, 0.50f};
        C.SecondaryStart = {HalfX - Dir * 0.05f, 0.55f};
        C.SecondaryEnd = {HalfX - Dir * 0.10f, 0.55f};
        C.BallStart = {HalfX, 0.45f};
        C.BallMid = {(HalfX + BasketX) * 0.5f, 0.48f};
        C.BallEnd = {BasketX, 0.50f};
        break;

    case EHighlightTemplate::FreeThrows:
        C.PrimaryStart = {FTX, 0.50f};
        C.PrimaryEnd = {FTX, 0.50f};
        C.SecondaryStart = {FTX - Dir * 0.03f, 0.35f};
        C.SecondaryEnd = {FTX - Dir * 0.03f, 0.65f};
        C.BallStart = {FTX, 0.50f};
        C.BallMid = {(FTX + BasketX) * 0.5f, 0.35f};
        C.BallEnd = {BasketX, 0.50f};
        break;

    case EHighlightTemplate::ClutchBasket:
        C.PrimaryStart = {DeepX, 0.20f};
        C.PrimaryEnd = {ArcX, 0.28f};
        C.SecondaryStart = {DeepX, 0.80f};
        C.SecondaryEnd = {ArcX, 0.72f};
        C.BallStart = {DeepX, 0.20f};
        C.BallMid = {(DeepX + BasketX) * 0.5f, 0.12f};
        C.BallEnd = {BasketX, 0.50f};
        break;

    case EHighlightTemplate::FinalPossession:
        C.PrimaryStart = {HalfX + Dir * 0.08f, 0.50f};
        C.PrimaryEnd = {BasketX - Dir * 0.04f, 0.46f};
        C.SecondaryStart = {HalfX + Dir * 0.05f, 0.25f};
        C.SecondaryEnd = {WingX, 0.30f};
        C.BallStart = {HalfX + Dir * 0.08f, 0.50f};
        C.BallMid = {WingX, 0.40f};
        C.BallEnd = {BasketX, 0.50f};
        break;

    default: // GenericFallback
        C.PrimaryStart = {HalfX, 0.40f};
        C.PrimaryEnd = {WingX, 0.45f};
        C.SecondaryStart = {HalfX, 0.60f};
        C.SecondaryEnd = {HalfX + Dir * 0.05f, 0.55f};
        C.BallStart = {HalfX, 0.40f};
        C.BallMid = {WingX, 0.35f};
        C.BallEnd = {BasketX, 0.50f};
        break;
    }

    return C;
}

UTacticalCourtWidget::FCourtPos UTacticalCourtWidget::LerpPos(FCourtPos A, FCourtPos B, float T)
{
    return {FMath::Lerp(A.X, B.X, T), FMath::Lerp(A.Y, B.Y, T)};
}

UTacticalCourtWidget::FCourtPos UTacticalCourtWidget::BezierPos(FCourtPos A, FCourtPos Mid, FCourtPos B, float T)
{
    FCourtPos AB = LerpPos(A, Mid, T);
    FCourtPos BC = LerpPos(Mid, B, T);
    return LerpPos(AB, BC, T);
}

void UTacticalCourtWidget::SetDotPosition(UCanvasPanelSlot* Slot, FCourtPos Pos,
    float CourtW, float CourtH, float DotSize)
{
    if (!Slot) { return; }
    Slot->SetOffsets(FMargin(
        Pos.X * CourtW - DotSize * 0.5f,
        Pos.Y * CourtH - DotSize * 0.5f,
        DotSize, DotSize));
    Slot->SetAutoSize(false);
}

void UTacticalCourtWidget::ApplyPositions(float Alpha)
{
    if (!CourtCanvas) { return; }

    const FGeometry& Geo = CourtCanvas->GetCachedGeometry();
    const FVector2D Size = Geo.GetLocalSize();
    if (Size.X < 1.0f || Size.Y < 1.0f) { return; }
    const float CourtW = Size.X;
    const float CourtH = Size.Y;

    const FHighlightCue* Cue = Presentation.Highlights.IsValidIndex(CurrentCueIndex)
        ? &Presentation.Highlights[CurrentCueIndex] : nullptr;

    bool bHomeTeam = true;
    if (Cue)
    {
        const FGuid& PossTeam = Cue->PossessionTeamId;
        bHomeTeam = PossTeam == Presentation.Home.TeamId || !PossTeam.IsValid();
    }

    FCueChoreography Choreo = Cue
        ? GetChoreography(Cue->Template, bHomeTeam)
        : GetChoreography(EHighlightTemplate::GenericFallback, true);

    float EasedAlpha = Alpha < 0.5f
        ? 2.0f * Alpha * Alpha
        : 1.0f - FMath::Pow(-2.0f * Alpha + 2.0f, 2.0f) * 0.5f;

    // Primary player (index 2 = center slot for the action player)
    TArray<TObjectPtr<UBorder>>& PrimaryTeamDots = bHomeTeam ? HomeDots : AwayDots;
    TArray<TObjectPtr<UBorder>>& DefenseTeamDots = bHomeTeam ? AwayDots : HomeDots;
    const TArray<FCourtPos>& PrimaryBase = bHomeTeam ? HomeBasePositions : AwayBasePositions;
    const TArray<FCourtPos>& DefenseBase = bHomeTeam ? AwayBasePositions : HomeBasePositions;

    for (int32 I = 0; I < 5; ++I)
    {
        if (!PrimaryTeamDots.IsValidIndex(I)) { continue; }
        UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(PrimaryTeamDots[I]->Slot);
        FCourtPos Pos = PrimaryBase[I];
        if (I == 2) // primary action player
        {
            Pos = LerpPos(Choreo.PrimaryStart, Choreo.PrimaryEnd, EasedAlpha);
        }
        else if (I == 3) // secondary action player
        {
            Pos = LerpPos(Choreo.SecondaryStart, Choreo.SecondaryEnd, EasedAlpha);
        }
        SetDotPosition(Slot, Pos, CourtW, CourtH, CourtStyle::DotSize);
    }

    for (int32 I = 0; I < 5; ++I)
    {
        if (!DefenseTeamDots.IsValidIndex(I)) { continue; }
        UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(DefenseTeamDots[I]->Slot);
        FCourtPos Pos = DefenseBase[I];
        if (I == 2)
        {
            float ReactAlpha = FMath::Min(EasedAlpha * 0.6f, 0.6f);
            Pos = LerpPos(DefenseBase[I], Choreo.PrimaryEnd, ReactAlpha);
        }
        SetDotPosition(Slot, Pos, CourtW, CourtH, CourtStyle::DotSize);
    }

    // Ball follows bezier curve
    if (BallDot)
    {
        UCanvasPanelSlot* BSlot = Cast<UCanvasPanelSlot>(BallDot->Slot);
        FCourtPos BallPos = BezierPos(Choreo.BallStart, Choreo.BallMid, Choreo.BallEnd, EasedAlpha);

        if (Cue && !Cue->bOutcome && EasedAlpha > 0.85f)
        {
            float Bounce = FMath::Sin((EasedAlpha - 0.85f) / 0.15f * PI) * 0.06f;
            BallPos.X -= Bounce * (bHomeTeam ? 1.0f : -1.0f);
            BallPos.Y += Bounce;
        }

        SetDotPosition(BSlot, BallPos, CourtW, CourtH, CourtStyle::BallSize);
    }
}

void UTacticalCourtWidget::RefreshScoreboard()
{
    if (!MatchupText || !ScoreText || !ClockText || !PlayText || !CueCountText) { return; }

    MatchupText->SetText(FText::FromString(FString::Printf(
        TEXT("%s  vs  %s"), *Presentation.Home.FullName, *Presentation.Away.FullName)));

    const FHighlightCue* Cue = Presentation.Highlights.IsValidIndex(CurrentCueIndex)
        ? &Presentation.Highlights[CurrentCueIndex] : nullptr;

    if (Cue)
    {
        ScoreText->SetText(FText::FromString(FString::Printf(
            TEXT("%d  -  %d"), Cue->HomeScoreAfter, Cue->AwayScoreAfter)));

        const FString Period = Cue->Period <= 4
            ? FString::Printf(TEXT("Q%d"), Cue->Period)
            : FString::Printf(TEXT("OT%d"), Cue->Period - 4);
        ClockText->SetText(FText::FromString(FString::Printf(
            TEXT("%s  %d:%02d"), *Period, Cue->ClockSeconds / 60, Cue->ClockSeconds % 60)));

        PlayText->SetText(FText::FromString(Cue->Description));

        CueCountText->SetText(FText::FromString(FString::Printf(
            TEXT("Highlight %d / %d"), CurrentCueIndex + 1, Presentation.Highlights.Num())));
    }
    else
    {
        ScoreText->SetText(FText::FromString(FString::Printf(
            TEXT("%d  -  %d"), Presentation.Result.HomeScore, Presentation.Result.AwayScore)));
        ClockText->SetText(FText::FromString(TEXT("FINAL")));
        PlayText->SetText(FText::FromString(TEXT("Broadcast complete")));
    }
}

void UTacticalCourtWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (bFinished || !Presentation.GameId.IsValid()) { return; }
    if (!Presentation.Highlights.IsValidIndex(CurrentCueIndex)) { return; }

    if (!bPaused)
    {
        const FHighlightCue& Cue = Presentation.Highlights[CurrentCueIndex];
        CueElapsed += InDeltaTime * PlaybackSpeed;
        const float Duration = FMath::Max(0.5f, Cue.PlaybackDuration);

        if (CueElapsed >= Duration)
        {
            AdvanceCue();
        }
        else
        {
            ApplyPositions(FMath::Clamp(CueElapsed / Duration, 0.0f, 1.0f));
        }
    }
    else
    {
        const FHighlightCue& Cue = Presentation.Highlights[CurrentCueIndex];
        const float Duration = FMath::Max(0.5f, Cue.PlaybackDuration);
        ApplyPositions(FMath::Clamp(CueElapsed / Duration, 0.0f, 1.0f));
    }
}

void UTacticalCourtWidget::AdvanceCue()
{
    ++CurrentCueIndex;
    CueElapsed = 0.0f;
    if (CurrentCueIndex >= Presentation.Highlights.Num())
    {
        bFinished = true;
        RefreshScoreboard();
        OnFinished.ExecuteIfBound();
    }
    else
    {
        RefreshScoreboard();
        ApplyPositions(0.0f);
    }
}

void UTacticalCourtWidget::HandlePause()
{
    bPaused = !bPaused;
    if (PauseLabel)
    {
        PauseLabel->SetText(FText::FromString(bPaused ? TEXT("PLAY") : TEXT("PAUSE")));
    }
}

void UTacticalCourtWidget::HandleSkip()
{
    if (!bFinished) { AdvanceCue(); }
}

void UTacticalCourtWidget::HandleSpeed()
{
    PlaybackSpeed = PlaybackSpeed < 1.5f ? 2.0f : PlaybackSpeed < 3.0f ? 4.0f : 1.0f;
    if (SpeedLabel)
    {
        SpeedLabel->SetText(FText::FromString(FString::Printf(TEXT("%.0fx"), PlaybackSpeed)));
    }
}

void UTacticalCourtWidget::HandleExit()
{
    bFinished = true;
    OnFinished.ExecuteIfBound();
}
