#include "BroadcastBallActor.h"

#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ABroadcastBallActor::ABroadcastBallActor()
{
    PrimaryActorTick.bCanEverTick = false;
    BallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ball"));
    SetRootComponent(BallMesh);
    BallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BallMesh->SetRelativeScale3D(FVector(0.12f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereMesh.Succeeded())
    {
        BallMesh->SetStaticMesh(SphereMesh.Object);
    }
}
