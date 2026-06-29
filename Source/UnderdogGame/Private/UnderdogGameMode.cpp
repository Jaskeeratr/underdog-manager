#include "UnderdogGameMode.h"
#include "UnderdogPlayerController.h"

AUnderdogGameMode::AUnderdogGameMode()
{
    PlayerControllerClass = AUnderdogPlayerController::StaticClass();
    DefaultPawnClass = nullptr;
}
