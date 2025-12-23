// Fill out your copyright notice in the Description page of Project Settings.

#include "AICS_CoverPoint.h"
#include "Components/ArrowComponent.h"

AAICS_CoverPoint::AAICS_CoverPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	FacingArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("FacingArrow"));
	FacingArrow->SetupAttachment(Root);

	ReservedBy.Reset();
}

bool AAICS_CoverPoint::Reserve(AActor* Requester)
{
	if (!IsValid(Requester))
	{
		return false;
	}

	if (!ReservedBy.IsValid())
	{
		ReservedBy.Reset();
	}

	if (ReservedBy.IsValid() && ReservedBy.Get() != Requester)
	{
		return false;
	}

	ReservedBy = Requester;
	return true;
}

void AAICS_CoverPoint::Release(AActor* Requester)
{
	if (!ReservedBy.IsValid())
	{
		ReservedBy.Reset();
		return;
	}

	if (Requester == nullptr || ReservedBy.Get() == Requester)
	{
		ReservedBy.Reset();
	}
}

bool AAICS_CoverPoint::IsReserved() const
{
	return ReservedBy.IsValid();
}

FVector AAICS_CoverPoint::GetFacingDirection() const
{
	return FacingArrow ? FacingArrow->GetForwardVector() : GetActorForwardVector();
}
