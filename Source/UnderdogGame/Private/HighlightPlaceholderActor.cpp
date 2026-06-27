#include "HighlightPlaceholderActor.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AHighlightPlaceholderActor::AHighlightPlaceholderActor()
{
    PrimaryActorTick.bCanEverTick = true;
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CapsuleMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BallMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    for (int32 Index = 0; Index < 10; ++Index)
    {
        UStaticMeshComponent* Marker = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Player_%02d"), Index));
        Marker->SetupAttachment(Root);
        Marker->SetStaticMesh(CapsuleMesh.Object);
        Marker->SetRelativeScale3D(FVector(0.32f, 0.32f, 0.9f));
        Marker->SetRelativeLocation(FVector((Index % 5) * 240.0f - 480.0f, Index < 5 ? -300.0f : 300.0f, 90.0f));
        PlayerMarkers.Add(Marker);
    }
    BallMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ball"));
    BallMarker->SetupAttachment(Root);
    BallMarker->SetStaticMesh(BallMesh.Object);
    BallMarker->SetRelativeScale3D(FVector(0.12f));
}

void AHighlightPlaceholderActor::PlayEvent(const FMatchEvent& Event)
{
    ActiveEvent = Event;
    Elapsed = 0.0f;
    bPlaying = true;
}

void AHighlightPlaceholderActor::Skip()
{
    bPlaying = false;
    Elapsed = 0.0f;
}

void AHighlightPlaceholderActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bPlaying || !BallMarker) { return; }
    Elapsed += DeltaSeconds;
    const float Alpha = FMath::Clamp(Elapsed / 2.5f, 0.0f, 1.0f);
    const FVector Start(-420.0f, -250.0f, 130.0f);
    const FVector End(420.0f, 250.0f, 320.0f);
    FVector Position = FMath::Lerp(Start, End, Alpha);
    Position.Z += FMath::Sin(Alpha * PI) * 350.0f;
    BallMarker->SetRelativeLocation(Position);
    if (Alpha >= 1.0f) { bPlaying = false; }
}
