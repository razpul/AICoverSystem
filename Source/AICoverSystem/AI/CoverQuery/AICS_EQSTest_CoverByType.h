// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "AICS_EQSTest_CoverByType.generated.h"

UCLASS()
class AICOVERSYSTEM_API UAICS_EQSTest_CoverByType : public UEnvQueryTest
{
	GENERATED_BODY()

public:
	UAICS_EQSTest_CoverByType();

	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;

protected:

	UPROPERTY(EditDefaultsOnly, Category = "cover")
	TSubclassOf<class UEnvQueryContext> ObjectiveLocationContext;

	// blackboard keys
	UPROPERTY(EditDefaultsOnly, Category = "threat")
	FName EnemyActorKeyName = TEXT("EnemyActor");

	UPROPERTY(EditDefaultsOnly, Category = "threat")
	FName LastKnownEnemyLocationKeyName = TEXT("LastKnownEnemyLocation");

	UPROPERTY(EditDefaultsOnly, Category = "threat")
	FName HasLOSKeyName = TEXT("HasLOS");

	// trace target height depends on cover type
	UPROPERTY(EditDefaultsOnly, Category = "cover")
	float LowCoverTraceHeight = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "cover")
	float HighCoverTraceHeight = 120.0f;

	UPROPERTY(EditDefaultsOnly, Category = "cover")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};
