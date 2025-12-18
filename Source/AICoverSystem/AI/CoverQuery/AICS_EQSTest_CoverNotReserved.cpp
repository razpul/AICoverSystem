// Fill out your copyright notice in the Description page of Project Settings.


#include "AICS_EQSTest_CoverNotReserved.h"

#include "EnvironmentQuery/Items/EnvQueryItemType_ActorBase.h"
#include "AICoverSystem/AI/CoverQuery/AICS_CoverPoint.h"


UAICS_EQSTest_CoverNotReserved::UAICS_EQSTest_CoverNotReserved()
{
	Cost = EEnvTestCost::Low;
	ValidItemType = UEnvQueryItemType_ActorBase::StaticClass();
}

void UAICS_EQSTest_CoverNotReserved::RunTest(FEnvQueryInstance& QueryInstance) const
{
    for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
    {
        AAICS_CoverPoint* Cover = Cast<AAICS_CoverPoint>(GetItemActor(QueryInstance, It.GetIndex()));

        const bool bAvailable = (Cover && !Cover->IsReserved());
        if (!bAvailable)
        {
            It.ForceItemState(EEnvItemStatus::Failed);
            continue;
        }

        It.SetScore(TestPurpose, FilterType, 1.0f, 0.0f, 1.0f);
    }
}
