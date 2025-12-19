// Fill out your copyright notice in the Description page of Project Settings.

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
    // name shown in the behavior tree editor
    NodeName = "Calc Reposition Location";
}

EBTNodeResult::Type UBTTask_CalcRepositionLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // get the basic ai refs we need to run this task
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    AAIController* AIC = OwnerComp.GetAIOwner();
    APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;

    // if anything important is missing, fail the task
    if (!BB || !AIC || !Pawn)
    {
        return EBTNodeResult::Failed;
    }

    // read the current cover point from the blackboard
    AAICS_CoverPoint* CurrentCover = Cast<AAICS_CoverPoint>(BB->GetValueAsObject(CurrentCoverKey.SelectedKeyName));
    if (!CurrentCover)
    {
        // no cover means no reposition spot to calculate
        BB->SetValueAsBool(HasRepositionLocationKey.SelectedKeyName, false);
        return EBTNodeResult::Succeeded;
    }

    // read the objective position (threat / target) and the current cover location
    const FVector ObjectiveLoc = BB->GetValueAsVector(ObjectiveLocationKey.SelectedKeyName);
    const FVector CoverLoc = CurrentCover->GetActorLocation();

    // if next cover is set, use it to decide which edge we should move toward
    // otherwise we just use the objective direction as a fallback
    const AActor* NextCoverActor = Cast<AActor>(BB->GetValueAsObject(NextCoverKey.SelectedKeyName));
    const FVector DirectionTargetLoc = NextCoverActor ? NextCoverActor->GetActorLocation() : ObjectiveLoc;

    // get nav system because we need to project candidate points onto navmesh
    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Pawn->GetWorld());
    if (!NavSys)
    {
        BB->SetValueAsBool(HasRepositionLocationKey.SelectedKeyName, false);
        return EBTNodeResult::Succeeded;
    }

    // figure out the direction "along" the cover wall
    // we take the cover facing arrow and get a perpendicular vector (sideways)
    const FVector Facing = CurrentCover->GetFacingDirection().GetSafeNormal2D();
    FVector AlongCover = FVector::CrossProduct(FVector::UpVector, Facing).GetSafeNormal2D();
    if (AlongCover.IsNearlyZero())
    {
        // safety fallback if facing is weird
        AlongCover = Pawn->GetActorRightVector().GetSafeNormal2D();
    }

    // decide which side is better (left/right) based on where we want to go next
    const FVector ToTarget = (DirectionTargetLoc - CoverLoc).GetSafeNormal2D();
    const float SideDot = FVector::DotProduct(AlongCover, ToTarget);

    // preferred side points more toward the next target
    const FVector PreferredSide = (SideDot >= 0.0f) ? AlongCover : -AlongCover;
    const FVector OtherSide = -PreferredSide;

    // helper: project a world position onto navmesh so move-to can actually reach it
    // (lambda function = small inline function we define inside this task)
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

    // trace heights:
    // threat height is usually "standing shot height"
    // target height depends on cover type (low cover = crouch height)
    const float ThreatHeight = HighTraceHeight;
    const float TargetHeight = (CurrentCover->CoverType == ECoverType::Low) ? LowTraceHeight : HighTraceHeight;

    // helper: line trace from objective -> test location
    // if it hits something, that means the test spot is blocked from the objective (good cover)
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

    // helper: scan outward on one side until cover breaks
    // last blocked point is the "edge", then we inset slightly so we still stay behind cover
    auto FindEdgeOnSide = [&](const FVector& SideDir, FVector& OutEdge) -> bool
        {
            bool bFoundAny = false;
            FVector LastBlocked = CoverLoc;

            // walk in small steps along the wall
            for (float Dist = 0.0f; Dist <= MaxEdgeScanDistance; Dist += EdgeScanStep)
            {
                const FVector Candidate = CoverLoc + SideDir * Dist;

                // keep points valid by projecting onto navmesh
                FVector Proj;
                if (!ProjectToNav(Candidate, Proj))
                {
                    continue;
                }

                // if still blocked, we can keep going
                // if it becomes unblocked, we hit the edge and stop
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

            // if we never found any blocked point, this side is not usable
            if (!bFoundAny)
            {
                return false;
            }

            // pull back a little so the ai doesn't stand on the exact edge
            FVector InsetLoc = LastBlocked - SideDir * EdgeInset;
            ProjectToNav(InsetLoc, InsetLoc);

            OutEdge = InsetLoc;
            return true;
        };

    // try to find an edge on the preferred side and the other side
    FVector EdgePref = FVector::ZeroVector;
    FVector EdgeOther = FVector::ZeroVector;

    const bool bHasPref = FindEdgeOnSide(PreferredSide, EdgePref);
    const bool bHasOther = FindEdgeOnSide(OtherSide, EdgeOther);

    // pick the best edge result
    // - if only one is valid, use it
    // - if both are valid, choose the one closer to the next cover / target direction
    bool bHasBest = false;
    FVector BestLoc = FVector::ZeroVector;

    if (bHasPref && !bHasOther)
    {
        bHasBest = true;
        BestLoc = EdgePref;
    }
    else if (!bHasPref && bHasOther)
    {
        bHasBest = true;
        BestLoc = EdgeOther;
    }
    else if (bHasPref && bHasOther)
    {
        const float DistPref = FVector::Dist2D(EdgePref, DirectionTargetLoc);
        const float DistOther = FVector::Dist2D(EdgeOther, DirectionTargetLoc);

        bHasBest = true;
        BestLoc = (DistPref <= DistOther) ? EdgePref : EdgeOther;
    }

    // optional debug draw so we can see what the task is doing in the world
    if (bDebugDraw)
    {
        const FVector CoverZ = CoverLoc + FVector(0, 0, TargetHeight);
        DrawDebugSphere(Pawn->GetWorld(), CoverZ, 18.0f, 12, FColor::Cyan, false, DebugDrawTime);

        // show the direction target (next cover if set, otherwise objective)
        if (NextCoverActor)
        {
            DrawDebugSphere(Pawn->GetWorld(), DirectionTargetLoc + FVector(0, 0, TargetHeight), 18.0f, 12, FColor::Yellow, false, DebugDrawTime);
            DrawDebugLine(Pawn->GetWorld(), CoverZ, DirectionTargetLoc + FVector(0, 0, TargetHeight), FColor::Yellow, false, DebugDrawTime, 0, 1.5f);
        }

        // show both candidate edges if they were found
        if (bHasPref)
        {
            DrawDebugSphere(Pawn->GetWorld(), EdgePref + FVector(0, 0, TargetHeight), 14.0f, 12, FColor::Blue, false, DebugDrawTime);
        }
        if (bHasOther)
        {
            DrawDebugSphere(Pawn->GetWorld(), EdgeOther + FVector(0, 0, TargetHeight), 14.0f, 12, FColor::Magenta, false, DebugDrawTime);
        }

        // show the final chosen edge
        if (bHasBest)
        {
            DrawDebugSphere(Pawn->GetWorld(), BestLoc + FVector(0, 0, TargetHeight), 24.0f, 16, FColor::White, false, DebugDrawTime);

            const FVector Start = ObjectiveLoc + FVector(0, 0, ThreatHeight);
            const FVector End = BestLoc + FVector(0, 0, TargetHeight);
            DrawDebugLine(Pawn->GetWorld(), Start, End, FColor::Green, false, DebugDrawTime, 0, 2.0f);
        }
    }

    // write results back to the blackboard so the bt can move to it
    BB->SetValueAsBool(HasRepositionLocationKey.SelectedKeyName, bHasBest);
    if (bHasBest)
    {
        BB->SetValueAsVector(RepositionLocationKey.SelectedKeyName, BestLoc);
    }

    return EBTNodeResult::Succeeded;
}
