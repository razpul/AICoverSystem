// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_FaceEnemy.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_FaceEnemy::UBTTask_FaceEnemy()
{
    NodeName = TEXT("Face Enemy");
}

EBTNodeResult::Type UBTTask_FaceEnemy::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIC = OwnerComp.GetAIOwner();
    if (!AIC)
    {
        return EBTNodeResult::Succeeded;
    }

    APawn* Pawn = AIC->GetPawn();
    if (!Pawn)
    {
        return EBTNodeResult::Succeeded;
    }

    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB) {
        return EBTNodeResult::Succeeded;
    }
    
    FVector LookAt = FVector::ZeroVector;
    bool bHasLookAt = false;

    AActor* EnemyActor = Cast<AActor>(BB->GetValueAsObject(EnemyActorKey.SelectedKeyName));
    if (EnemyActor) {
        
        LookAt = EnemyActor->GetActorLocation();
        bHasLookAt = true;
    }
    else if (bUseLastKnownLocationIfNoEnemy && LastKnownEnemyLocationKey.SelectedKeyName != NAME_None)
    {
        LookAt = BB->GetValueAsVector(LastKnownEnemyLocationKey.SelectedKeyName);
        bHasLookAt = true;
    }

    if (!bHasLookAt)
    {
        return EBTNodeResult::Succeeded;
    }

    const FVector From = Pawn->GetActorLocation();
    FVector Dir = (LookAt - From);
    Dir.Z = 0.0f;

    if (Dir.IsNearlyZero())
    {
        return EBTNodeResult::Succeeded;
    }

    const FRotator NewYawRot = Dir.Rotation();
    Pawn->SetActorRotation(FRotator(0.0f, NewYawRot.Yaw, 0.0f));

    if (bDebugDraw)
    {
        const FVector End = From + (Dir.GetSafeNormal() * 250.0f);
        DrawDebugLine(Pawn->GetWorld(), From, End, FColor::Yellow, false, DebugDuration, 0, 2.0f);
    }

    return EBTNodeResult::Succeeded;
}
