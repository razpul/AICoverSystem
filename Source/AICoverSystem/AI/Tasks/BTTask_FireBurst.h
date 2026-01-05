// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FireBurst.generated.h"

/**
 * 
 */
UCLASS()
class AICOVERSYSTEM_API UBTTask_FireBurst : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FireBurst();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "blackboard")
	FBlackboardKeySelector EnemyActorKey;

	UPROPERTY(EditAnywhere, Category = "debug")
	bool bDrawDebugTracer = true;

	UPROPERTY(EditAnywhere, Category = "debug", meta = (EditCondition = "bDrawDebugTracer"))
	float DebugDuration = 0.2f;

	UPROPERTY(EditAnywhere, Category = "debug", meta = (EditCondition = "bDrawDebugTracer"))
	float DebugThickness = 2.0f;

	UPROPERTY(EditAnywhere, Category = "debug", meta = (EditCondition = "bDrawDebugTracer"))
	float DebugSpreadRadius = 35.0f;

	UPROPERTY(EditAnywhere, Category = "debug")
	bool bLogFire = false;
};