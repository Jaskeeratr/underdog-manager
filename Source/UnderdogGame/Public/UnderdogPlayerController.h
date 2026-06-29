#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UnderdogPlayerController.generated.h"

UCLASS()
class UNDERDOGGAME_API AUnderdogPlayerController : public APlayerController
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;
};
