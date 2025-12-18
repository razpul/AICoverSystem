// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "AICS_EQSTest_MakeProgress.generated.h"

/**
 * 
 */
UCLASS()
class AICOVERSYSTEM_API UAICS_EQSTest_MakeProgress : public UEnvQueryTest
{
	GENERATED_BODY()

public:
	UAICS_EQSTest_MakeProgress();

protected:
	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;

	UPROPERTY(EditDefaultsOnly, Category = "Progress")
	TSubclassOf<UEnvQueryContext> ObjectiveLocationContext;

	UPROPERTY(EditDefaultsOnly, Category = "Progress")
	float MinAdvance = 50.f;	
};
