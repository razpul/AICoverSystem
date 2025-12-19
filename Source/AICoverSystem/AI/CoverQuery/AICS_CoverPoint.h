// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AICS_CoverPoint.generated.h"

class UArrowComponent;

UENUM(BlueprintType)
enum class ECoverType : uint8
{
	Low UMETA(DisplayName = "Low"),
	High UMETA(DisplayName = "High")
};

UCLASS()
class AICOVERSYSTEM_API AAICS_CoverPoint : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AAICS_CoverPoint();

	UFUNCTION(BlueprintCallable, Category = "Cover")
	bool Reserve(AActor* Requester);

	UFUNCTION(BlueprintCallable, Category = "Cover")
	void Release(AActor* Requester);

	UFUNCTION(BlueprintCallable, Category = "Cover")
	bool IsReserved() const;

	UFUNCTION(BlueprintPure, Category = "Cover")
	AActor* GetReservedBy() const { return ReservedBy; }

	UFUNCTION(BlueprintPure, Category = "Cover")
	FVector GetCoverLocation() const { return GetActorLocation(); }

	UFUNCTION(BlueprintPure, Category = "Cover")
	FVector GetFacingDirection() const;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Cover")
	ECoverType CoverType = ECoverType::High;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> FacingArrow;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Cover")
	TObjectPtr<AActor> ReservedBy;

public:


};
