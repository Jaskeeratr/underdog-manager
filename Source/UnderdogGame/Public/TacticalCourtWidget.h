#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UnderdogCoreTypes.h"
#include "TacticalCourtWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UCanvasPanelSlot;
class UTextBlock;

DECLARE_DELEGATE(FOnTacticalBroadcastFinished);

UCLASS()
class UNDERDOGGAME_API UTacticalCourtWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitializeBroadcast(const FMatchPresentationPackage& InPresentation);
    FOnTacticalBroadcastFinished OnFinished;

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    struct FCourtPos { float X; float Y; };

    struct FCueChoreography
    {
        FCourtPos PrimaryStart;
        FCourtPos PrimaryEnd;
        FCourtPos SecondaryStart;
        FCourtPos SecondaryEnd;
        FCourtPos BallStart;
        FCourtPos BallMid;
        FCourtPos BallEnd;
    };

    static FCueChoreography GetChoreography(EHighlightTemplate Template, bool bHomeTeam);
    void AdvanceCue();
    void ApplyPositions(float Alpha);
    void RefreshScoreboard();
    void SetDotPosition(UCanvasPanelSlot* Slot, FCourtPos Pos, float CourtW, float CourtH, float DotSize);
    FCourtPos LerpPos(FCourtPos A, FCourtPos B, float T);
    FCourtPos BezierPos(FCourtPos A, FCourtPos Mid, FCourtPos B, float T);
    UBorder* MakeDot(float Size, const FLinearColor& Color);
    UBorder* MakeCourtLine(float X, float Y, float W, float H, const FLinearColor& Color);
    UTextBlock* MakeLabel(const FString& Text, int32 Size, const FLinearColor& Color, bool bBold = false);

    UFUNCTION() void HandlePause();
    UFUNCTION() void HandleSkip();
    UFUNCTION() void HandleSpeed();
    UFUNCTION() void HandleExit();

    FMatchPresentationPackage Presentation;
    int32 CurrentCueIndex = INDEX_NONE;
    float CueElapsed = 0.0f;
    float PlaybackSpeed = 1.0f;
    bool bPaused = false;
    bool bFinished = false;

    UPROPERTY() TObjectPtr<UCanvasPanel> CourtCanvas;
    UPROPERTY() TArray<TObjectPtr<UBorder>> HomeDots;
    UPROPERTY() TArray<TObjectPtr<UBorder>> AwayDots;
    UPROPERTY() TObjectPtr<UBorder> BallDot;
    UPROPERTY() TObjectPtr<UTextBlock> ScoreText;
    UPROPERTY() TObjectPtr<UTextBlock> ClockText;
    UPROPERTY() TObjectPtr<UTextBlock> PlayText;
    UPROPERTY() TObjectPtr<UTextBlock> MatchupText;
    UPROPERTY() TObjectPtr<UTextBlock> PauseLabel;
    UPROPERTY() TObjectPtr<UTextBlock> SpeedLabel;
    UPROPERTY() TObjectPtr<UTextBlock> CueCountText;

    TArray<FCourtPos> HomeBasePositions;
    TArray<FCourtPos> AwayBasePositions;
};
