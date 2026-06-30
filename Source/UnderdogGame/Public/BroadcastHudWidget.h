#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BroadcastHudWidget.generated.h"

class ABroadcastArenaDirector;
class UButton;
class UTextBlock;

UCLASS()
class UNDERDOGGAME_API UBroadcastHudWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitializeForDirector(ABroadcastArenaDirector* InDirector);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    UFUNCTION() void HandlePause();
    UFUNCTION() void HandleSkip();
    UFUNCTION() void HandleSpeed();
    UFUNCTION() void HandleExit();
    void RefreshLabels();

    UPROPERTY() TObjectPtr<ABroadcastArenaDirector> Director;
    UPROPERTY() TObjectPtr<UTextBlock> MatchupText;
    UPROPERTY() TObjectPtr<UTextBlock> ScoreText;
    UPROPERTY() TObjectPtr<UTextBlock> ClockText;
    UPROPERTY() TObjectPtr<UTextBlock> CueText;
    UPROPERTY() TObjectPtr<UTextBlock> PauseText;
    UPROPERTY() TObjectPtr<UTextBlock> SpeedText;
};
