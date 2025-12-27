// Fill out your copyright notice in the Description page of Project Settings.


#include "AICS_CoverAIController.h"

#include "AICoverSystem/AI/Character/AICS_CoverAICharacter.h"
#include "AICoverSystem/Objective/AICS_ObjectivePoint.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EngineUtils.h"

#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

AAICS_CoverAIController::AAICS_CoverAIController()
{
	BlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComp"));

    PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));
    SetPerceptionComponent(*PerceptionComp);

    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

    SightConfig->SightRadius = SightRadius;
    SightConfig->LoseSightRadius = LoseSightRadius;
    SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionDegrees;
    SightConfig->SetMaxAge(SightMaxAge);

    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

    PerceptionComp->ConfigureSense(*SightConfig);
    PerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());
    PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AAICS_CoverAIController::HandleTargetPerceptionUpdated);
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

void AAICS_CoverAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (!BlackboardComp || !Actor)
    {
        return;
    }

    if (Actor->IsA(AAICS_CoverAICharacter::StaticClass()))
    {
        return;
    }

    if (!EnemyTag.IsNone() && !Actor->ActorHasTag(EnemyTag))
    {
        return;
    }

    const UObject* CurrentEnemyObj = BlackboardComp->GetValueAsObject(EnemyActorKeyName);
    const bool bIsCurrentEnemy = (CurrentEnemyObj == Actor);

    if (Stimulus.WasSuccessfullySensed())
    {
        if (!EnemyActorKeyName.IsNone())
        {
            BlackboardComp->SetValueAsObject(EnemyActorKeyName, Actor);
        }

        if (!LastKnownEnemyLocationKeyName.IsNone())
        {
            BlackboardComp->SetValueAsVector(LastKnownEnemyLocationKeyName, Stimulus.StimulusLocation);
        }

        const bool bHadLOS = BlackboardComp->GetValueAsBool(HasLOSKeyName);
        if (!HasLOSKeyName.IsNone())
        {
            BlackboardComp->SetValueAsBool(HasLOSKeyName, true);
        }

        if (bLogPerception && (!bIsCurrentEnemy || !bHadLOS))
        {
            UE_LOG(LogTemp, Log, TEXT("perception: saw %s"), *GetNameSafe(Actor));
        }

        if (bDebugDrawPerception && GetPawn())
        {
            DrawDebugLine(GetWorld(), GetPawn()->GetActorLocation(), Actor->GetActorLocation(), FColor::Green, false, DebugDrawDuration, 0, DebugDrawThickness);
            DrawDebugSphere(GetWorld(), Actor->GetActorLocation(), 20.0f, 12, FColor::Green, false, DebugDrawDuration);
        }

        return;
    }

    if (bIsCurrentEnemy)
    {
        if (!HasLOSKeyName.IsNone())
        {
            BlackboardComp->ClearValue(HasLOSKeyName);
        }

        if (!LastKnownEnemyLocationKeyName.IsNone())
        {
            BlackboardComp->SetValueAsVector(LastKnownEnemyLocationKeyName, Stimulus.StimulusLocation);
        }

        if (bLogPerception)
        {
            UE_LOG(LogTemp, Log, TEXT("perception: lost %s"), *GetNameSafe(Actor));
        }

        if (bDebugDrawPerception && GetPawn())
        {
            DrawDebugLine(GetWorld(), GetPawn()->GetActorLocation(), Stimulus.StimulusLocation, FColor::Red, false, DebugDrawDuration, 0, DebugDrawThickness);
            DrawDebugSphere(GetWorld(), Stimulus.StimulusLocation, 20.0f, 12, FColor::Red, false, DebugDrawDuration);
        }
    }
}
