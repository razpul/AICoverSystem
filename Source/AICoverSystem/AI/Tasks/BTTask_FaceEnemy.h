// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FaceEnemy.generated.h"

/**
 * 
 */
UCLASS()
class AICOVERSYSTEM_API UBTTask_FaceEnemy : public UBTTaskNode
{
	GENERATED_BODY()

	UBTTask_FaceEnemy();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector EnemyActorKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	bool bUseLastKnownLocationIfNoEnemy = true;

	UPROPERTY(EditAnywhere, Category = "Blackboard", meta = (EditCondition = "bUseLastKnownLocationIfNoEnemy"))
	FBlackboardKeySelector LastKnownEnemyLocationKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	bool bDebugDraw = false;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	float DebugDuration = 0.5f;
};
