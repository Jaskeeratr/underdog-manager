#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UnderdogCoreTypes.h"
#include "BroadcastArenaDirector.generated.h"

class ABroadcastAthleteActor;
class ABroadcastBallActor;
class ACameraActor;
class UStaticMeshComponent;

DECLARE_MULTICAST_DELEGATE(FOnBroadcastFinished);

UCLASS()
class UNDERDOGGAME_API ABroadcastArenaDirector : public AActor
{
    GENERATED_BODY()

public:
    ABroadcastArenaDirector();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    bool InitializeBroadcast(const FMatchPresentationPackage& InPresentation, APlayerController* Viewer);
    void TogglePause();
    void SkipCue();
    void SetPlaybackSpeed(float InSpeed);
    void StopBroadcast();

    bool IsPaused() const { return bPaused; }
    float GetPlaybackSpeed() const { return PlaybackSpeed; }
    int32 GetCueIndex() const { return CurrentCueIndex; }
    int32 GetCueCount() const { return Presentation.Highlights.Num(); }
    const FHighlightCue* GetCurrentCue() const;
    const FMatchPresentationPackage& GetPresentation() const { return Presentation; }

    FOnBroadcastFinished OnBroadcastFinished;

private:
    void SpawnArena();
    void StartCurrentCue();
    void ApplyCue(float Alpha);
    void ApplyCamera(const FHighlightCue& Cue);
    FVector GetAthleteBasePosition(int32 Index) const;

    UPROPERTY() TObjectPtr<UStaticMeshComponent> Court;
    UPROPERTY() TArray<TObjectPtr<ABroadcastAthleteActor>> Athletes;
    UPROPERTY() TObjectPtr<ABroadcastBallActor> Ball;
    UPROPERTY() TObjectPtr<ACameraActor> BroadcastCamera;
    UPROPERTY() TObjectPtr<APlayerController> ViewingController;

    FMatchPresentationPackage Presentation;
    int32 CurrentCueIndex = INDEX_NONE;
    float CueElapsed = 0.0f;
    float PlaybackSpeed = 1.0f;
    bool bPaused = false;
    bool bFinishing = false;
};
