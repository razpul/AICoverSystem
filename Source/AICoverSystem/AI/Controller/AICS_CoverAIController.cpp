// Fill out your copyright notice in the Description page of Project Settings.


#include "AICS_CoverAIController.h"

#include "AICoverSystem/AI/Character/AICS_CoverAICharacter.h"
#include "AICoverSystem/Objective/AICS_ObjectivePoint.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EngineUtils.h"

AAICS_CoverAIController::AAICS_CoverAIController()
{
	BlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComp"));
}

void AAICS_CoverAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);


    AAICS_CoverAICharacter* CoverChar = Cast<AAICS_CoverAICharacter>(InPawn);
    if (!CoverChar)
    {
        UE_LOG(LogTemp, Warning, TEXT("Possessed pawn is not CoverAICharacter."));
        return;
    }

    UBehaviorTree* BTAsset = CoverChar->GetBehaviorTree();
    if (!BTAsset)
    {
        UE_LOG(LogTemp, Warning, TEXT("No BehaviorTreeAsset set on character."));
        return;
    }

    if (!BTAsset->BlackboardAsset)
    {
        UE_LOG(LogTemp, Warning, TEXT("BehaviorTree has no BlackboardAsset assigned."));
        return;
    }

    UBlackboardComponent* BB = nullptr;
    if (!UseBlackboard(BTAsset->BlackboardAsset, BB) || !BB)
    {
        UE_LOG(LogTemp, Warning, TEXT("UseBlackboard failed."));
        return;
    }

    BlackboardComp = BB;

    if (!RunBehaviorTree(BTAsset))
    {
        UE_LOG(LogTemp, Warning, TEXT("RunBehaviorTree failed."));
        return;
    }

    if (!ObjectiveActor.IsValid())
    {
        for (TActorIterator<AAICS_ObjectivePoint> It(GetWorld()); It; ++It)
        {
            ObjectiveActor = *It;
            break;
        }
    }

    UpdateObjectiveInBlackboard();
}

void AAICS_CoverAIController::SetObjectiveActor(AAICS_ObjectivePoint* InObjective)
{
    ObjectiveActor = InObjective;
    UpdateObjectiveInBlackboard();
}

void AAICS_CoverAIController::UpdateObjectiveInBlackboard()
{
    if (!BlackboardComp)
    {
        return;
    }

    if (!ObjectiveActor.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("ObjectiveActor is not set"));
        return;
    }

    const FVector ObjectiveLoc = ObjectiveActor->GetActorLocation();

    if (!ObjectiveActorKeyName.IsNone())
    {
        BlackboardComp->SetValueAsVector(ObjectiveLocationKeyName, ObjectiveLoc);
    }

    if (!ObjectiveActorKeyName.IsNone())
    {
        BlackboardComp->SetValueAsObject(ObjectiveActorKeyName, ObjectiveActor.Get());
    }
}
