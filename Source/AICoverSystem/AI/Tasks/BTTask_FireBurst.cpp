// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_FireBurst.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_FireBurst::UBTTask_FireBurst()
{
	NodeName = TEXT("Fire Burst");
}

EBTNodeResult::Type UBTTask_FireBurst::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	if (!AICon)
	{
		return EBTNodeResult::Succeeded;
	}

	APawn* Pawn = AICon->GetPawn();
	if (!Pawn)
	{
		return EBTNodeResult::Succeeded;
	}

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
	{
		return EBTNodeResult::Succeeded;
	}

	AActor* EnemyActor = Cast<AActor>(BB->GetValueAsObject(EnemyActorKey.SelectedKeyName));
	if (!EnemyActor)
	{
		return EBTNodeResult::Succeeded;
	}

	const FVector Start = Pawn->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f);

	FVector End = EnemyActor->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f);

	if (DebugSpreadRadius > 0.0f)
	{
		End.X += FMath::FRandRange(-DebugSpreadRadius, DebugSpreadRadius);
		End.Y += FMath::FRandRange(-DebugSpreadRadius, DebugSpreadRadius);
	}

	if (bDrawDebugTracer)
	{
		DrawDebugLine(Pawn->GetWorld(), Start, End, FColor::Cyan, false, DebugDuration, 0, DebugThickness);
		DrawDebugSphere(Pawn->GetWorld(), End, 10.0f, 8, FColor::Cyan, false, DebugDuration);
	}

	if (bLogFire)
	{
		UE_LOG(LogTemp, Log, TEXT("fire burst: %s -> %s"), *GetNameSafe(Pawn), *GetNameSafe(EnemyActor));
	}

	return EBTNodeResult::Succeeded;
}
