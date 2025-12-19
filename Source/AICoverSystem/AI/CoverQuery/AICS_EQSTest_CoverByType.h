// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "AICS_EQSTest_CoverByType.generated.h"

/**
 * 
 */
UCLASS()
class AICOVERSYSTEM_API UAICS_EQSTest_CoverByType : public UEnvQueryTest
{
	GENERATED_BODY()

public:
	UAICS_EQSTest_CoverByType();

	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Cover")
	TSubclassOf<class UEnvQueryContext> ObjectiveLocationContext;

	UPROPERTY(EditDefaultsOnly, Category = "Cover")
	float LowCoverTraceHeight = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Cover")
	float HighCoverTraceHeight = 120.f;

	UPROPERTY(EditDefaultsOnly, Category = "Cover")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
	
};
