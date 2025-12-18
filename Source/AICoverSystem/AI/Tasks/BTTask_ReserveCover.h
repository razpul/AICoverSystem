// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ReserveCover.generated.h"


UCLASS()
class AICOVERSYSTEM_API UBTTask_ReserveCover : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_ReserveCover();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetCoverKey;

    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector CurrentCoverKey;
};
