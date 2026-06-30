#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "UnderdogCoreTypes.h"
#include "UnderdogSaveGame.generated.h"

UCLASS()
class UNDERDOGGAME_API UUnderdogSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    static constexpr int32 CurrentSchemaVersion = 6;
    UPROPERTY(SaveGame) int32 SchemaVersion = CurrentSchemaVersion;
    UPROPERTY(SaveGame) FDateTime SavedAtUtc;
    UPROPERTY(SaveGame) FLeagueState League;
};
