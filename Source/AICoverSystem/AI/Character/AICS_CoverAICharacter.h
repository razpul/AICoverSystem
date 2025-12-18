// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AICS_CoverAICharacter.generated.h"


class UBehaviorTree;

UCLASS()
class AICOVERSYSTEM_API AAICS_CoverAICharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AAICS_CoverAICharacter();

	UFUNCTION(BlueprintPure, Category = "AI")
	UBehaviorTree* GetBehaviorTree() const { return BehaviorTreeAsset; }

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	// Called when the game starts or when spawned
	//virtual void BeginPlay() override;

public:	
	//// Called every frame
	//virtual void Tick(float DeltaTime) override;

	//// Called to bind functionality to input
	//virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
