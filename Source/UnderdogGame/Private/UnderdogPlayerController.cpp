#include "UnderdogPlayerController.h"
#include "ManagementDashboardWidget.h"

void AUnderdogPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (!IsLocalController()) { return; }

    UManagementDashboardWidget* Dashboard = CreateWidget<UManagementDashboardWidget>(
        this, UManagementDashboardWidget::StaticClass());
    if (Dashboard)
    {
        Dashboard->AddToViewport(100);
        bShowMouseCursor = true;
        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture(false);
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        SetInputMode(InputMode);
    }
}
