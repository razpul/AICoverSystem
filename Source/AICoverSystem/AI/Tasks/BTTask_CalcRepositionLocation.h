// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_CalcRepositionLocation.generated.h"

UCLASS()
class AICOVERSYSTEM_API UBTTask_CalcRepositionLocation : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_CalcRepositionLocation();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector CurrentCoverKey;

    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector ObjectiveLocationKey;

    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector RepositionLocationKey;

    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector HasRepositionLocationKey;

    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector NextCoverKey;

    UPROPERTY(EditAnywhere, Category = "Reposition")
    float NavProjectExtent = 200.0f;

    UPROPERTY(EditAnywhere, Category = "Reposition")
    float LowTraceHeight = 80.0f;

    UPROPERTY(EditAnywhere, Category = "Reposition")
    float HighTraceHeight = 120.0f;

    UPROPERTY(EditAnywhere, Category = "Reposition")
    float EdgeInset = 15.0f;

    UPROPERTY(EditAnywhere, Category = "Reposition")
    float MaxNavSnapDistance2D = 75.0f;

    UPROPERTY(EditAnywhere, Category = "Reposition")
    float MaxLateralDetourRatio = 1.35f;

    UPROPERTY(EditAnywhere, Category = "Reposition")
    float MinFirstStepDot = 0.25f;

    UPROPERTY(EditAnywhere, Category = "Reposition")
    int32 MaxEdgeInwardTries = 6;

    UPROPERTY(EditAnywhere, Category = "Reposition")
    float EdgeInwardStep = 25.0f;

    UPROPERTY(EditAnywhere, Category = "Debug")
    bool bDebugDraw = false;

    UPROPERTY(EditAnywhere, Category = "Debug")
    float DebugDrawTime = 1.0f;
};
