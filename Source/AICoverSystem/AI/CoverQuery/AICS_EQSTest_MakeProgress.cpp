#include "AICS_EQSTest_MakeProgress.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/Pawn.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_ActorBase.h" 

UAICS_EQSTest_MakeProgress::UAICS_EQSTest_MakeProgress()
{
    Cost = EEnvTestCost::Low;
    ValidItemType = UEnvQueryItemType_ActorBase::StaticClass();
}

void UAICS_EQSTest_MakeProgress::RunTest(FEnvQueryInstance& QueryInstance) const
{
    if (!ObjectiveLocationContext)
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

    UObject* OwnerObj = QueryInstance.Owner.Get();
    if (!OwnerObj)
    {
        return;
    }

    const APawn* Pawn = nullptr;
    if (const AAIController* AIC = Cast<AAIController>(OwnerObj))
    {
        Pawn = AIC->GetPawn();
    }
    else
    {
        Pawn = Cast<APawn>(OwnerObj);
    }

    FVector QuerierLoc;
    if (Pawn)
    {
        QuerierLoc = Pawn->GetActorLocation();
    }
    else if (const AActor* OwnerActor = Cast<AActor>(OwnerObj))
    {
        QuerierLoc = OwnerActor->GetActorLocation();
    }
    else
    {
        return;
    }

    const FVector ToObj = (ObjectiveLoc - QuerierLoc);
    const FVector ToObj2D(ToObj.X, ToObj.Y, 0.0f);
    const FVector DirToObj = ToObj2D.GetSafeNormal();

    if (DirToObj.IsNearlyZero())
    {
        return;
    }

    for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
    {
        const AActor* ItemActor = GetItemActor(QueryInstance, It.GetIndex());
        if (!ItemActor)
        {
            It.ForceItemState(EEnvItemStatus::Failed);
            continue;
        }

        const FVector ItemLoc = ItemActor->GetActorLocation();
        const FVector MoveVec2D(ItemLoc.X - QuerierLoc.X, ItemLoc.Y - QuerierLoc.Y, 0.0f);

        const float Forward = FVector::DotProduct(MoveVec2D, DirToObj);

        if (Forward < -50.0f)
        {
            It.ForceItemState(EEnvItemStatus::Failed);
            continue;
        }

        const float Score = FMath::Max(0.0f, Forward);
        It.SetScore(TestPurpose, FilterType, Score, 0.0f, 3000.0f);
    }
}
