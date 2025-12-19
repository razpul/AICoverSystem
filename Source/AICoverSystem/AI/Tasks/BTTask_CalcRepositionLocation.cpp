#include "BTTask_CalcRepositionLocation.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "GameFramework/Pawn.h"
#include "DrawDebugHelpers.h"

#include <cfloat>

#include "AICoverSystem/AI/CoverQuery/AICS_CoverPoint.h"

UBTTask_CalcRepositionLocation::UBTTask_CalcRepositionLocation()
{
    NodeName = "Calc Reposition Location";
}

EBTNodeResult::Type UBTTask_CalcRepositionLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    AAIController* AIC = OwnerComp.GetAIOwner();
    APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;

    if (!BB || !AIC || !Pawn)
    {
        return EBTNodeResult::Failed;
    }

    AAICS_CoverPoint* CurrentCover = Cast<AAICS_CoverPoint>(BB->GetValueAsObject(CurrentCoverKey.SelectedKeyName));
    if (!CurrentCover)
    {
        BB->SetValueAsBool(HasRepositionLocationKey.SelectedKeyName, false);
        return EBTNodeResult::Succeeded;
    }

    const FVector ObjectiveLoc = BB->GetValueAsVector(ObjectiveLocationKey.SelectedKeyName);
    const FVector CoverLoc = CurrentCover->GetActorLocation();

    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Pawn->GetWorld());
    if (!NavSys)
    {
        BB->SetValueAsBool(HasRepositionLocationKey.SelectedKeyName, false);
        return EBTNodeResult::Succeeded;
    }

    const FVector Facing = CurrentCover->GetFacingDirection().GetSafeNormal2D();
    FVector AlongCover = FVector::CrossProduct(FVector::UpVector, Facing).GetSafeNormal2D();
    if (AlongCover.IsNearlyZero())
    {
        AlongCover = Pawn->GetActorRightVector().GetSafeNormal2D();
    }

    const FVector ToObj = (ObjectiveLoc - CoverLoc).GetSafeNormal2D();
    const float SideDot = FVector::DotProduct(AlongCover, ToObj);
    const FVector PreferredSide = (SideDot >= 0.0f) ? AlongCover : -AlongCover;
    const FVector OtherSide = -PreferredSide;

    auto ProjectToNav = [&](const FVector& InLoc, FVector& OutLoc) -> bool
        {
            FNavLocation Projected;
            const FVector Extent(NavProjectExtent, NavProjectExtent, NavProjectExtent);
            if (NavSys->ProjectPointToNavigation(InLoc, Projected, Extent))
            {
                OutLoc = Projected.Location;
                return true;
            }
            return false;
        };

    const float ThreatHeight = HighTraceHeight; 
    const float TargetHeight = (CurrentCover->CoverType == ECoverType::Low) ? LowTraceHeight : HighTraceHeight;

    auto IsBlockedFromObjective = [&](const FVector& TestLoc) -> bool
        {
            const FVector Start = ObjectiveLoc + FVector(0, 0, ThreatHeight);
            const FVector End = TestLoc + FVector(0, 0, TargetHeight);

            FHitResult Hit;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(RepositionTrace), false);
            Params.AddIgnoredActor(Pawn);

            const bool bHit = Pawn->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
            return bHit;
        };


    auto FindEdgeOnSide = [&](const FVector& SideDir, FVector& OutEdge) -> bool
        {
            bool bFoundAny = false;
            FVector LastBlocked = CoverLoc;

            for (float Dist = 0.0f; Dist <= MaxEdgeScanDistance; Dist += EdgeScanStep)
            {
                const FVector Candidate = CoverLoc + SideDir * Dist;

                FVector Proj;
                if (!ProjectToNav(Candidate, Proj))
                {
                    continue;
                }

                const bool bBlocked = IsBlockedFromObjective(Proj);
                if (bBlocked)
                {
                    bFoundAny = true;
                    LastBlocked = Proj;
                }
                else
                {
                    break;
                }
            }

            if (!bFoundAny)
            {
                return false;
            }

            FVector InsetLoc = LastBlocked - SideDir * EdgeInset;
            ProjectToNav(InsetLoc, InsetLoc);

            OutEdge = InsetLoc;
            return true;
        };

    FVector BestLoc = FVector::ZeroVector;
    bool bHasBest = false;

    if (FindEdgeOnSide(PreferredSide, BestLoc))
    {
        bHasBest = true;
    }
    else if (FindEdgeOnSide(OtherSide, BestLoc))
    {
        bHasBest = true;
    }

    if (bDebugDraw)
    {
        DrawDebugSphere(Pawn->GetWorld(), CoverLoc + FVector(0, 0, TargetHeight), 18.0f, 12, FColor::Cyan, false, DebugDrawTime);

        if (bHasBest)
        {
            DrawDebugSphere(Pawn->GetWorld(), BestLoc + FVector(0, 0, TargetHeight), 24.0f, 16, FColor::White, false, DebugDrawTime);

            const FVector Start = ObjectiveLoc + FVector(0, 0, ThreatHeight);
            const FVector End = BestLoc + FVector(0, 0, TargetHeight);
            DrawDebugLine(Pawn->GetWorld(), Start, End, FColor::Green, false, DebugDrawTime, 0, 2.0f);
        }
    }

    BB->SetValueAsBool(HasRepositionLocationKey.SelectedKeyName, bHasBest);
    if (bHasBest)
    {
        BB->SetValueAsVector(RepositionLocationKey.SelectedKeyName, BestLoc);
    }

    return EBTNodeResult::Succeeded;
}
