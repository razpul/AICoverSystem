// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AICS_CoverAIController.generated.h"

class UBlackboardComponent;
class UBehaviourTree;
class AAICS_ObjectivePoint;
class UIAIPerceptionComponent;
class UAISenseConfig_Sight;


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

	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

protected:
	// blackboard
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBlackboardComponent> BlackboardComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Blackboard")
	FName ObjectiveLocationKeyName = TEXT("ObjectiveLocation");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Blackboard")
	FName ObjectiveActorKeyName = TEXT("ObjectiveActor");

	// perception bb keys
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Blackboard")
	FName EnemyActorKeyName = TEXT("EnemyActor");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Blackboard")
	FName LastKnownEnemyLocationKeyName = TEXT("LastKnownEnemyLocation");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Blackboard")
	FName HasLOSKeyName = TEXT("HasLOS");

	// object ref
	UPROPERTY()
	TWeakObjectPtr<AAICS_ObjectivePoint> ObjectiveActor;

	// perception
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAIPerceptionComponent> PerceptionComp;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception")
	float SightRadius = 3500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception")
	float LoseSightRadius = 4500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception")
	float PeripheralVisionDegrees = 70.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception")
	float SightMaxAge = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception")
	FName EnemyTag = TEXT("enemy");

	// debug 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception")
	bool bLogPerception = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception|Debug")
	bool bDebugDrawPerception = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception|Debug")
	float DebugDrawDuration = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception|Debug")
	float DebugDrawThickness = 2.0f;


};
