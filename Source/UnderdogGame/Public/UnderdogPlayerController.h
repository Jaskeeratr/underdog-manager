#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UnderdogCoreTypes.h"
#include "UnderdogPlayerController.generated.h"

class UManagementDashboardWidget;
class UTacticalCourtWidget;

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
    UPROPERTY() TObjectPtr<UTacticalCourtWidget> CourtWidget;
};
