// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_SetStanceFromCurrentCover.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"

#include "AICoverSystem/AI/CoverQuery/AICS_CoverPoint.h"


UBTTask_SetStanceFromCurrentCover::UBTTask_SetStanceFromCurrentCover()
{
	NodeName = "Set Stance From Current Cover";
}

EBTNodeResult::Type UBTTask_SetStanceFromCurrentCover::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

	AAICS_CoverPoint* CurrentCover = Cast<AAICS_CoverPoint>(BB->GetValueAsObject(CurrentCoverKey.SelectedKeyName));
	if (!CurrentCover)
	{
		return EBTNodeResult::Failed;
	}

	const bool bShouldCrouch = (CurrentCover->CoverType == ECoverType::Low);

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
