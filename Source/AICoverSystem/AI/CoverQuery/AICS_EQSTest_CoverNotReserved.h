// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "AICS_EQSTest_CoverNotReserved.generated.h"

/**
 * 
 */
UCLASS()
class AICOVERSYSTEM_API UAICS_EQSTest_CoverNotReserved : public UEnvQueryTest
{
	GENERATED_BODY()

public:
	UAICS_EQSTest_CoverNotReserved();

protected:
	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;
	
};
