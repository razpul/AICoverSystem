// Fill out your copyright notice in the Description page of Project Settings.


#include "AICS_CoverPoint.h"
#include "Components/ArrowComponent.h"

// Sets default values
AAICS_CoverPoint::AAICS_CoverPoint()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	FacingArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("FacingArrow"));
	FacingArrow->SetupAttachment(Root);

	ReservedBy = nullptr;

}

bool AAICS_CoverPoint::Reserve(AActor* Requester)
{
	if (!IsValid(Requester))
	{
		return false;
	}

	if (IsReserved() && ReservedBy != Requester)
	{
		return false;
	}

	ReservedBy = Requester;
	return true;
}

void AAICS_CoverPoint::Release(AActor* Requester)
{
	if (!IsReserved())
	{
		return;
	}

	if (Requester == nullptr || ReservedBy == Requester)
	{
		ReservedBy = nullptr;
	}
}

bool AAICS_CoverPoint::IsReserved() const
{
	return IsValid(ReservedBy);
}

FVector AAICS_CoverPoint::GetFacingDirection() const
{
	return FacingArrow ? FacingArrow->GetForwardVector() : GetActorForwardVector();
}



