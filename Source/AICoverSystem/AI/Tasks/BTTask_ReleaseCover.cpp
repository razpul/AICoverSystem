// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_ReleaseCover.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"

#include "AICoverSystem/AI/CoverQuery/AICS_CoverPoint.h"

UBTTask_ReleaseCover::UBTTask_ReleaseCover()
{
	NodeName = "Release Cover";
}

EBTNodeResult::Type UBTTask_ReleaseCover::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;

	if (!BB || !Pawn) 
	{
		return EBTNodeResult::Failed;
	}

	AAICS_CoverPoint* CurrentCover = Cast<AAICS_CoverPoint>(BB->GetValueAsObject(CurrentCoverKey.SelectedKeyName));
	if (CurrentCover)
	{
		CurrentCover->Release(Pawn);
	}

	BB->ClearValue(CurrentCoverKey.SelectedKeyName);

	return EBTNodeResult::Succeeded;
}
