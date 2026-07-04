#include "UnderdogPlayerController.h"
#include "ManagementDashboardWidget.h"
#include "TacticalCourtWidget.h"

void AUnderdogPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (!IsLocalController()) { return; }

    Dashboard = CreateWidget<UManagementDashboardWidget>(
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

bool AUnderdogPlayerController::StartBroadcast(const FMatchPresentationPackage& Presentation)
{
    if (CourtWidget || Presentation.Highlights.IsEmpty()) { return false; }

    CourtWidget = CreateWidget<UTacticalCourtWidget>(this, UTacticalCourtWidget::StaticClass());
    if (!CourtWidget) { return false; }

    CourtWidget->InitializeBroadcast(Presentation);
    CourtWidget->OnFinished.BindUObject(this, &AUnderdogPlayerController::HandleBroadcastFinished);
    CourtWidget->AddToViewport(200);

    if (Dashboard) { Dashboard->SetVisibility(ESlateVisibility::Collapsed); }
    return true;
}

void AUnderdogPlayerController::HandleBroadcastFinished()
{
    if (CourtWidget)
    {
        CourtWidget->OnFinished.Unbind();
        CourtWidget->RemoveFromParent();
        CourtWidget = nullptr;
    }
    if (Dashboard)
    {
        Dashboard->SetVisibility(ESlateVisibility::Visible);
        Dashboard->ReturnFromBroadcast();
    }
}
