#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "UnderdogGameInstance.generated.h"

UCLASS(Config=Game)
class UNDERDOGGAME_API UUnderdogGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Underdog") int64 DefaultLeagueSeed = 20260627;
};
