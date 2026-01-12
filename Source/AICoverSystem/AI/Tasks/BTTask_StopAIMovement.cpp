// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_StopAIMovement.h"

#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"

UBTTask_StopAIMovement::UBTTask_StopAIMovement()
{
	NodeName = "Stop AI Movement";
}

EBTNodeResult::Type UBTTask_StopAIMovement::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;

	if (!AIC || !Pawn)
	{
		return EBTNodeResult::Failed;
	}

	AIC->StopMovement();

	if (UCharacterMovementComponent* MoveComp = Pawn->FindComponentByClass<UCharacterMovementComponent>())
	{
		MoveComp->StopMovementImmediately();
	}

	return EBTNodeResult::Succeeded;
}