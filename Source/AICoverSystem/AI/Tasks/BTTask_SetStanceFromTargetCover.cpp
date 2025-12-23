// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_SetStanceFromTargetCover.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"

#include "AICoverSystem/AI/CoverQuery/AICS_CoverPoint.h"

UBTTask_SetStanceFromTargetCover::UBTTask_SetStanceFromTargetCover()
{
	NodeName = "set stance from target cover";
}

EBTNodeResult::Type UBTTask_SetStanceFromTargetCover::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AIC = OwnerComp.GetAIOwner();

	if (!BB || !AIC)
	{
		return EBTNodeResult::Failed;
	}
	
	ACharacter* Char = Cast<ACharacter>(AIC->GetPawn());
	if (!Char)
	{
		return EBTNodeResult::Failed;
	}

	AAICS_CoverPoint* TargetCover = Cast<AAICS_CoverPoint>(BB->GetValueAsObject(TargetCoverKey.SelectedKeyName));
	if (!TargetCover)
	{
		return EBTNodeResult::Succeeded;
	}

	const bool bShouldCrouch = (TargetCover->CoverType == ECoverType::Low);

	if (bShouldCrouch)
	{
		Char->Crouch();
	}
	else
	{
		Char->UnCrouch();
	}

	if (!WantsCrouchKey.SelectedKeyName.IsNone())
	{
		BB->SetValueAsBool(WantsCrouchKey.SelectedKeyName, bShouldCrouch);
	}

	return EBTNodeResult::Succeeded;
}
