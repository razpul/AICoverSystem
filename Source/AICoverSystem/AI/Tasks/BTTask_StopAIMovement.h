// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_StopAIMovement.generated.h"

/**
 * 
 */
UCLASS()
class AICOVERSYSTEM_API UBTTask_StopAIMovement : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_StopAIMovement();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;	
};
