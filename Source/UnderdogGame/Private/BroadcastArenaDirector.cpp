#include "BroadcastArenaDirector.h"

#include "BroadcastAthleteActor.h"
#include "BroadcastBallActor.h"
#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetMathLibrary.h"
#include "UObject/ConstructorHelpers.h"

ABroadcastArenaDirector::ABroadcastArenaDirector()
{
    PrimaryActorTick.bCanEverTick = true;
    Court = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Court"));
    SetRootComponent(Court);
    Court->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Court->SetRelativeScale3D(FVector(14.0f, 7.5f, 0.05f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        Court->SetStaticMesh(CubeMesh.Object);
    }
}

bool ABroadcastArenaDirector::InitializeBroadcast(
    const FMatchPresentationPackage& InPresentation, APlayerController* Viewer)
{
    if (!Viewer || !InPresentation.GameId.IsValid() || InPresentation.Highlights.IsEmpty())
    {
        return false;
    }

    Presentation = InPresentation;
    ViewingController = Viewer;
    SpawnArena();
    CurrentCueIndex = 0;
    StartCurrentCue();
    return Athletes.Num() == 10 && Ball && BroadcastCamera;
}

void ABroadcastArenaDirector::SpawnArena()
{
    UWorld* World = GetWorld();
    if (!World) { return; }

    for (int32 Index = 0; Index < 10; ++Index)
    {
        ABroadcastAthleteActor* Athlete = World->SpawnActor<ABroadcastAthleteActor>(
            GetAthleteBasePosition(Index), FRotator::ZeroRotator);
        if (Athlete)
        {
            Athlete->Configure(Index < 5, Index % 5);
            Athletes.Add(Athlete);
        }
    }

    Ball = World->SpawnActor<ABroadcastBallActor>(FVector(0.0f, 0.0f, 145.0f), FRotator::ZeroRotator);
    BroadcastCamera = World->SpawnActor<ACameraActor>(FVector(-900.0f, 1050.0f, 850.0f), FRotator::ZeroRotator);
    if (BroadcastCamera && ViewingController)
    {
        BroadcastCamera->SetActorRotation(
            UKismetMathLibrary::FindLookAtRotation(BroadcastCamera->GetActorLocation(), FVector::ZeroVector));
        ViewingController->SetViewTarget(BroadcastCamera);
    }
}

FVector ABroadcastArenaDirector::GetAthleteBasePosition(int32 Index) const
{
    const bool bHome = Index < 5;
    const int32 Slot = Index % 5;
    const float X = bHome ? -360.0f + Slot * 95.0f : 360.0f - Slot * 95.0f;
    const float Y = -220.0f + Slot * 110.0f;
    return GetActorLocation() + FVector(X, Y, 105.0f);
}

const FHighlightCue* ABroadcastArenaDirector::GetCurrentCue() const
{
    return Presentation.Highlights.IsValidIndex(CurrentCueIndex)
        ? &Presentation.Highlights[CurrentCueIndex]
        : nullptr;
}

void ABroadcastArenaDirector::StartCurrentCue()
{
    CueElapsed = 0.0f;
    for (int32 Index = 0; Index < Athletes.Num(); ++Index)
    {
        if (Athletes[Index]) { Athletes[Index]->SetActorLocation(GetAthleteBasePosition(Index)); }
    }
    if (Ball) { Ball->SetActorLocation(GetActorLocation() + FVector(-150.0f, 0.0f, 145.0f)); }
    if (const FHighlightCue* Cue = GetCurrentCue()) { ApplyCamera(*Cue); }
}

void ABroadcastArenaDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const FHighlightCue* Cue = GetCurrentCue();
    if (!Cue || bPaused || bFinishing) { return; }

    CueElapsed += DeltaSeconds * PlaybackSpeed;
    const float Duration = FMath::Max(0.5f, Cue->PlaybackDuration);
    const float Alpha = FMath::Clamp(CueElapsed / Duration, 0.0f, 1.0f);
    ApplyCue(Alpha);

    if (Alpha >= 1.0f)
    {
        ++CurrentCueIndex;
        if (CurrentCueIndex >= Presentation.Highlights.Num())
        {
            StopBroadcast();
        }
        else
        {
            StartCurrentCue();
        }
    }
}

void ABroadcastArenaDirector::ApplyCue(float Alpha)
{
    const FHighlightCue* Cue = GetCurrentCue();
    if (!Cue || Athletes.IsEmpty() || !Ball) { return; }

    ABroadcastAthleteActor* Primary = Athletes[CurrentCueIndex % Athletes.Num()];
    const FVector Start = GetActorLocation() + FVector(-320.0f, -80.0f, 105.0f);
    const FVector Finish = GetActorLocation() + FVector(410.0f, 0.0f, 105.0f);
    if (Primary)
    {
        Primary->SetActorLocation(FMath::Lerp(Start, Finish, Alpha));
    }

    FVector BallLocation = FMath::Lerp(Start + FVector(0.0f, 0.0f, 60.0f),
        Finish + FVector(0.0f, 0.0f, 150.0f), Alpha);
    const bool bShot = Cue->Template == EHighlightTemplate::ThreePointer
        || Cue->Template == EHighlightTemplate::DriveAndFinish
        || Cue->Template == EHighlightTemplate::AssistedBasket
        || Cue->Template == EHighlightTemplate::ClutchBasket
        || Cue->Template == EHighlightTemplate::FinalPossession
        || Cue->Template == EHighlightTemplate::FreeThrows;
    if (bShot)
    {
        BallLocation.Z += FMath::Sin(Alpha * PI) * 300.0f;
    }
    Ball->SetActorLocation(BallLocation);
}

void ABroadcastArenaDirector::ApplyCamera(const FHighlightCue& Cue)
{
    if (!BroadcastCamera) { return; }

    FVector CameraLocation(-900.0f, 1050.0f, 850.0f);
    switch (Cue.Camera)
    {
    case ECameraPreset::CloseUp: CameraLocation = FVector(-250.0f, 500.0f, 300.0f); break;
    case ECameraPreset::HighAngle: CameraLocation = FVector(0.0f, 150.0f, 1450.0f); break;
    case ECameraPreset::ReverseAngle: CameraLocation = FVector(850.0f, -900.0f, 600.0f); break;
    case ECameraPreset::Baseline: CameraLocation = FVector(650.0f, 0.0f, 420.0f); break;
    case ECameraPreset::FollowBall: CameraLocation = FVector(-450.0f, 520.0f, 360.0f); break;
    case ECameraPreset::Wide: CameraLocation = FVector(-1100.0f, 1250.0f, 950.0f); break;
    default: break;
    }
    CameraLocation += GetActorLocation();
    BroadcastCamera->SetActorLocation(CameraLocation);
    BroadcastCamera->SetActorRotation(
        UKismetMathLibrary::FindLookAtRotation(CameraLocation, GetActorLocation() + FVector(0.0f, 0.0f, 100.0f)));
}

void ABroadcastArenaDirector::TogglePause() { bPaused = !bPaused; }

void ABroadcastArenaDirector::SkipCue()
{
    if (bFinishing) { return; }
    ++CurrentCueIndex;
    if (CurrentCueIndex >= Presentation.Highlights.Num()) { StopBroadcast(); }
    else { StartCurrentCue(); }
}

void ABroadcastArenaDirector::SetPlaybackSpeed(float InSpeed)
{
    PlaybackSpeed = FMath::Clamp(InSpeed, 0.5f, 4.0f);
}

void ABroadcastArenaDirector::StopBroadcast()
{
    if (bFinishing) { return; }
    bFinishing = true;
    OnBroadcastFinished.Broadcast();
}

void ABroadcastArenaDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    for (ABroadcastAthleteActor* Athlete : Athletes)
    {
        if (IsValid(Athlete)) { Athlete->Destroy(); }
    }
    if (IsValid(Ball)) { Ball->Destroy(); }
    if (IsValid(BroadcastCamera)) { BroadcastCamera->Destroy(); }
    Super::EndPlay(EndPlayReason);
}
