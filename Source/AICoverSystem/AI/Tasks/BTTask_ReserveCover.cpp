// Fill out your copyright notice in the Description page of Project Settings.

#include "BTTask_ReserveCover.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"

#include "AICoverSystem/AI/CoverQuery/AICS_CoverPoint.h"

UBTTask_ReserveCover::UBTTask_ReserveCover()
{
	NodeName = "reserve cover";
}

EBTNodeResult::Type UBTTask_ReserveCover::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;

	if (!BB || !Pawn)
	{
		return EBTNodeResult::Failed;
	}

	AAICS_CoverPoint* TargetCover = Cast<AAICS_CoverPoint>(BB->GetValueAsObject(TargetCoverKey.SelectedKeyName));
	if (!TargetCover)
	{
		return EBTNodeResult::Failed;
	}

	if (!TargetCover->Reserve(Pawn))
	{
		BB->ClearValue(TargetCoverKey.SelectedKeyName);
		return EBTNodeResult::Failed;
	}

	return EBTNodeResult::Succeeded;
}
