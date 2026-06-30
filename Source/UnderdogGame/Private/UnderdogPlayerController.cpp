#include "UnderdogPlayerController.h"
#include "BroadcastArenaDirector.h"
#include "BroadcastHudWidget.h"
#include "Engine/World.h"
#include "ManagementDashboardWidget.h"

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
    if (BroadcastDirector || Presentation.Highlights.IsEmpty() || !GetWorld()) { return false; }

    PreviousViewTarget = GetViewTarget();
    BroadcastDirector = GetWorld()->SpawnActor<ABroadcastArenaDirector>(
        FVector::ZeroVector, FRotator::ZeroRotator);
    if (!BroadcastDirector || !BroadcastDirector->InitializeBroadcast(Presentation, this))
    {
        if (BroadcastDirector) { BroadcastDirector->Destroy(); }
        BroadcastDirector = nullptr;
        return false;
    }

    BroadcastDirector->OnBroadcastFinished.AddUObject(this, &AUnderdogPlayerController::HandleBroadcastFinished);
    BroadcastHud = CreateWidget<UBroadcastHudWidget>(this, UBroadcastHudWidget::StaticClass());
    if (BroadcastHud)
    {
        BroadcastHud->InitializeForDirector(BroadcastDirector);
        BroadcastHud->AddToViewport(200);
    }
    if (Dashboard) { Dashboard->SetVisibility(ESlateVisibility::Collapsed); }
    return true;
}

void AUnderdogPlayerController::HandleBroadcastFinished()
{
    if (BroadcastHud)
    {
        BroadcastHud->RemoveFromParent();
        BroadcastHud = nullptr;
    }
    if (PreviousViewTarget) { SetViewTarget(PreviousViewTarget); }
    if (BroadcastDirector)
    {
        BroadcastDirector->OnBroadcastFinished.RemoveAll(this);
        BroadcastDirector->Destroy();
        BroadcastDirector = nullptr;
    }
    if (Dashboard)
    {
        Dashboard->SetVisibility(ESlateVisibility::Visible);
        Dashboard->ReturnFromBroadcast();
    }
}
