#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UnderdogCoreTypes.h"
#include "UnderdogPlayerController.generated.h"

class ABroadcastArenaDirector;
class UBroadcastHudWidget;
class UManagementDashboardWidget;

UCLASS()
class UNDERDOGGAME_API AUnderdogPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    bool StartBroadcast(const FMatchPresentationPackage& Presentation);

protected:
    virtual void BeginPlay() override;

private:
    void HandleBroadcastFinished();

    UPROPERTY() TObjectPtr<UManagementDashboardWidget> Dashboard;
    UPROPERTY() TObjectPtr<UBroadcastHudWidget> BroadcastHud;
    UPROPERTY() TObjectPtr<ABroadcastArenaDirector> BroadcastDirector;
    UPROPERTY() TObjectPtr<AActor> PreviousViewTarget;
};
