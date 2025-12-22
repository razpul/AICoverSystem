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
    // How big is the navigation search box
    UPROPERTY(EditAnywhere, Category = "Reposition")
    float NavProjectExtent = 200.0f;
    // Z height used for low cover tests (crouch height line trace)
    UPROPERTY(EditAnywhere, Category = "Reposition")
    float LowTraceHeight = 80.0f;
    // Z height used for high cover text (standing height line trace)
    UPROPERTY(EditAnywhere, Category = "Reposition")
    float HighTraceHeight = 120.0f;
    // How far to pull back from exact edge so AI stays safely behind cover
    UPROPERTY(EditAnywhere, Category = "Reposition")
    float EdgeInset = 15.0f;
    // After projecting to navmesh, reject the point if nav snapped it too far away in 2D space
    UPROPERTY(EditAnywhere, Category = "Reposition")
    float MaxNavSnapDistance2D = 75.0f;
    // Rejects edge points that cause big detours (path edge must be close to straight line)
    UPROPERTY(EditAnywhere, Category = "Reposition")
    float MaxLateralDetourRatio = 1.35f;
    // Makes sure path starts by moving roughly in the side direction we want (stops backtracking)
    UPROPERTY(EditAnywhere, Category = "Reposition")
    float MinFirstStepDot = 0.25f;
    // How many times to try stepping inward from edge if the first edge point is bad
    UPROPERTY(EditAnywhere, Category = "Reposition")
    int32 MaxEdgeInwardTries = 6;
    // How far each inward retry step moves when searching for a better edge point
    UPROPERTY(EditAnywhere, Category = "Reposition")
    float EdgeInwardStep = 25.0f;
    // Toggles debug drawing for reposition task
    UPROPERTY(EditAnywhere, Category = "Debug")
    bool bDebugDraw = false;
    // Sets debug draw time for this task
    UPROPERTY(EditAnywhere, Category = "Debug")
    float DebugDrawTime = 1.0f;
};
