// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryContext_CurrentCover.generated.h"

UCLASS()
class AICOVERSYSTEM_API UEnvQueryContext_CurrentCover : public UEnvQueryContext
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, Category = "EQS")
    FName CurrentCoverBBKey = TEXT("CurrentCover");

    virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};

