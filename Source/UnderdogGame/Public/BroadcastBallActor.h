#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BroadcastBallActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class UNDERDOGGAME_API ABroadcastBallActor : public AActor
{
    GENERATED_BODY()

public:
    ABroadcastBallActor();

private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> BallMesh;
};
