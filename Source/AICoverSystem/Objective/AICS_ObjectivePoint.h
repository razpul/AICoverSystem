// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AICS_ObjectivePoint.generated.h"

class USphereComponent;
class UBillboardComponent;

UCLASS()
class AICOVERSYSTEM_API AAICS_ObjectivePoint : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAICS_ObjectivePoint();

	UFUNCTION(BlueprintPure, Category = "Objective")
	FVector GetObjectiveLocation() const { return GetActorLocation(); }

	UFUNCTION(BlueprintPure, Category = "Objective")
	float GetObjectiveRadius() const;

protected:
	//// Called when the game starts or when spawned
	//virtual void BeginPlay() override;
	//
	//// Called every frame
	//virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> RadiusSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBillboardComponent> Billboard;

	UFUNCTION()
	void OnRadiusBeginOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

public:	

};
