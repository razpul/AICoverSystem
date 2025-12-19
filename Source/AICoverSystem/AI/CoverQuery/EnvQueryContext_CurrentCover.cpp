// Fill out your copyright notice in the Description page of Project Settings.

#include "EnvQueryContext_CurrentCover.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"

#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"

void UEnvQueryContext_CurrentCover::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
    UObject* OwnerObj = QueryInstance.Owner.Get();
    if (!OwnerObj)
    {
        return;
    }

    AAIController* AIC = Cast<AAIController>(OwnerObj);
    if (!AIC)
    {
        if (APawn* Pawn = Cast<APawn>(OwnerObj))
        {
            AIC = Cast<AAIController>(Pawn->GetController());
        }
    }

    if (!AIC)
    {
        return;
    }

    UBlackboardComponent* BB = AIC->GetBlackboardComponent();
    if (!BB)
    {
        return;
    }

    AActor* CurrentCoverActor = Cast<AActor>(BB->GetValueAsObject(CurrentCoverBBKey));
    if (!CurrentCoverActor)
    {
        return;
    }

    UEnvQueryItemType_Actor::SetContextHelper(ContextData, CurrentCoverActor);
}

