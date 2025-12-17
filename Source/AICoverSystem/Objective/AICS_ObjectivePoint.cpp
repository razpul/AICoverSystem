// Fill out your copyright notice in the Description page of Project Settings.


#include "AICS_ObjectivePoint.h"
#include "Components/SphereComponent.h"
#include "Components/BillboardComponent.h"

// Sets default values
AAICS_ObjectivePoint::AAICS_ObjectivePoint()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	Billboard->SetupAttachment(Root);

	RadiusSphere = CreateDefaultSubobject<USphereComponent>(TEXT("RadiusSphere"));
	RadiusSphere->SetupAttachment(Root);
	RadiusSphere->SetSphereRadius(400.f);

	RadiusSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	RadiusSphere->SetCollisionObjectType(ECC_WorldDynamic);
	RadiusSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	RadiusSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RadiusSphere->SetGenerateOverlapEvents(true);

	RadiusSphere->OnComponentBeginOverlap.AddDynamic(this, &AAICS_ObjectivePoint::OnRadiusBeginOverlap);
}

float AAICS_ObjectivePoint::GetObjectiveRadius() const
{
	return RadiusSphere ? RadiusSphere->GetScaledSphereRadius() : 0.0f;
}

//// Called when the game starts or when spawned
//void AAICS_ObjectivePoint::BeginPlay()
//{
//	Super::BeginPlay();
//	
//}
//
//// Called every frame
//void AAICS_ObjectivePoint::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//
//}

void AAICS_ObjectivePoint::OnRadiusBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsValid(OtherActor))
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Objective reached by : %s"), *OtherActor->GetName());
}

