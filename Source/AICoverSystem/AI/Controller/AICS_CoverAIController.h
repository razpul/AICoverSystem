// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AICS_CoverAIController.generated.h"

class UBlackboardComponent;
class UBehaviourTree;
class AAICS_ObjectivePoint;


UCLASS()
class AICOVERSYSTEM_API AAICS_CoverAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	AAICS_CoverAIController();

	virtual void OnPossess(APawn* InPawn) override;

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetObjectiveActor(AAICS_ObjectivePoint* InObjective);

protected:
	void UpdateObjectiveInBlackboard();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBlackboardComponent> BlackboardComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Blackboard")
	FName ObjectiveLocationKeyName = TEXT("ObjectiveLocation");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Blackboard")
	FName ObjectiveActorKeyName = TEXT("ObjectiveActor");

	UPROPERTY()
	TWeakObjectPtr<AAICS_ObjectivePoint> ObjectiveActor;




};
