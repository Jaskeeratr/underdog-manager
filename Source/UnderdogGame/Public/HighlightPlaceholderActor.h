#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UnderdogCoreTypes.h"
#include "HighlightPlaceholderActor.generated.h"

class UStaticMeshComponent;

UCLASS(BlueprintType)
class UNDERDOGGAME_API AHighlightPlaceholderActor : public AActor
{
    GENERATED_BODY()

public:
    AHighlightPlaceholderActor();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category="Underdog|Highlights") void PlayEvent(const FMatchEvent& Event);
    UFUNCTION(BlueprintCallable, Category="Underdog|Highlights") void Skip();
    UFUNCTION(BlueprintPure, Category="Underdog|Highlights") bool IsPlaying() const { return bPlaying; }

private:
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> PlayerMarkers;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> BallMarker;
    FMatchEvent ActiveEvent;
    float Elapsed = 0.0f;
    bool bPlaying = false;
};
