#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BroadcastAthleteActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class UNDERDOGGAME_API ABroadcastAthleteActor : public AActor
{
    GENERATED_BODY()

public:
    ABroadcastAthleteActor();
    void Configure(bool bInHomeTeam, int32 JerseyIndex);

private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
};
