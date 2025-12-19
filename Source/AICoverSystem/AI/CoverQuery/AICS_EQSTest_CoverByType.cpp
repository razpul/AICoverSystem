#include "AICS_EQSTest_CoverByType.h"

#include "DrawDebugHelpers.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_ActorBase.h"

#include "AICoverSystem/AI/CoverQuery/AICS_CoverPoint.h"

UAICS_EQSTest_CoverByType::UAICS_EQSTest_CoverByType()
{
    Cost = EEnvTestCost::Low;
    ValidItemType = UEnvQueryItemType_ActorBase::StaticClass();

    TestPurpose = EEnvTestPurpose::Filter;          
    FilterType = EEnvTestFilterType::Range;        
}

void UAICS_EQSTest_CoverByType::RunTest(FEnvQueryInstance& QueryInstance) const
{
    if (!ObjectiveLocationContext)
    {
        return;
    }

    UWorld* World = QueryInstance.World;
    if (!World)
    {
        return;
    }

    TArray<FVector> ObjectiveLocs;
    QueryInstance.PrepareContext(ObjectiveLocationContext, ObjectiveLocs);
    if (ObjectiveLocs.Num() == 0)
    {
        return;
    }
    const FVector ObjectiveLoc = ObjectiveLocs[0];

    AActor* QuerierActor = nullptr;
    {
        TArray<AActor*> QuerierActors;
        QueryInstance.PrepareContext(UEnvQueryContext_Querier::StaticClass(), QuerierActors);
        if (QuerierActors.Num() > 0)
        {
            QuerierActor = QuerierActors[0];
        }
    }

    FCollisionQueryParams Params(SCENE_QUERY_STAT(EQSCoverByType), false);
    if (QuerierActor)
    {
        Params.AddIgnoredActor(QuerierActor);
    }

    const float ThreatHeight = HighCoverTraceHeight;

    for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
    {
        const AActor* ItemActor = GetItemActor(QueryInstance, It.GetIndex());
        const AAICS_CoverPoint* CoverPoint = Cast<AAICS_CoverPoint>(ItemActor);

        if (!CoverPoint)
        {
            It.ForceItemState(EEnvItemStatus::Failed);
            continue;
        }

        const float TargetHeight =
            (CoverPoint->CoverType == ECoverType::Low) ? LowCoverTraceHeight : HighCoverTraceHeight;

        const FVector Start = ObjectiveLoc + FVector(0.0f, 0.0f, ThreatHeight);
        const FVector End = CoverPoint->GetActorLocation() + FVector(0.0f, 0.0f, TargetHeight);

        FHitResult Hit;
        const bool bBlocked = World->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Params);

        const float Value = bBlocked ? 1.0f : 0.0f;
        It.SetScore(TestPurpose, FilterType, Value, 0.0f, 1.0f);

        if (!bBlocked)
        {
            It.ForceItemState(EEnvItemStatus::Failed);
        }

    }
}
