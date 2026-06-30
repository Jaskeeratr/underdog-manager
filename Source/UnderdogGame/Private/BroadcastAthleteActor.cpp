#include "BroadcastAthleteActor.h"

#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ABroadcastAthleteActor::ABroadcastAthleteActor()
{
    PrimaryActorTick.bCanEverTick = false;
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    SetRootComponent(Body);
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->SetRelativeScale3D(FVector(0.42f, 0.42f, 0.95f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyMesh(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (BodyMesh.Succeeded())
    {
        Body->SetStaticMesh(BodyMesh.Object);
    }
}

void ABroadcastAthleteActor::Configure(bool bInHomeTeam, int32 JerseyIndex)
{
    const FLinearColor TeamColor = bInHomeTeam
        ? FLinearColor(0.05f, 0.32f, 0.95f)
        : FLinearColor(0.95f, 0.16f, 0.08f);
    Body->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(TeamColor));
    Tags.AddUnique(bInHomeTeam ? TEXT("HomeAthlete") : TEXT("AwayAthlete"));
    Tags.AddUnique(FName(*FString::Printf(TEXT("Jersey%d"), JerseyIndex + 1)));
}
