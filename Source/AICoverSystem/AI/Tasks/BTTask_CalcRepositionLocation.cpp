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

    const FVector Facing = CurrentCover->GetFacingDirection().GetSafeNormal2D();
    FVector AlongCover = FVector::CrossProduct(FVector::UpVector, Facing).GetSafeNormal2D();
    if (AlongCover.IsNearlyZero())
    {
        AlongCover = Pawn->GetActorRightVector().GetSafeNormal2D();
    }

    const FVector CandidateA = CoverLoc + AlongCover * LateralOffset;
    const FVector CandidateB = CoverLoc - AlongCover * LateralOffset;

    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Pawn->GetWorld());
    if (!NavSys)
    {
        BB->SetValueAsBool(HasRepositionLocationKey.SelectedKeyName, false);
        return EBTNodeResult::Succeeded;
    }

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

    auto IsBlockedFromObjective = [&](const FVector& TestLoc) -> bool
        {
            const FVector Start = ObjectiveLoc + FVector(0, 0, TraceHeightOffset);
            const FVector End = TestLoc + FVector(0, 0, TraceHeightOffset);

            FHitResult Hit;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(RepositionTrace), true);
            Params.AddIgnoredActor(Pawn);

            const bool bHit = Pawn->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
            return bHit;
        };

    auto ScoreCandidate = [&](const FVector& ProjectedLoc) -> float
        {
            float Score = 0.0f;

            const bool bBlocked = IsBlockedFromObjective(ProjectedLoc);
            if (bRequireBlockedFromObjective && !bBlocked)
            {
                return -FLT_MAX; 
            }

            if (bBlocked)
            {
                Score += 1000.0f; 
            }

            const FVector ToObj = (ObjectiveLoc - CoverLoc).GetSafeNormal2D();
            const FVector Delta = (ProjectedLoc - CoverLoc);
            const float Forward = FVector::DotProduct(Delta.GetSafeNormal2D(), ToObj);
            Score += Forward * 50.0f;

            const float Dist = FVector::Dist2D(ProjectedLoc, CoverLoc);
            Score -= Dist * 0.5f;

            return Score;
        };

    auto DrawTraceResult = [&](const FVector& Proj, const FColor& Col)
        {
            const FVector Start = ObjectiveLoc + FVector(0, 0, TraceHeightOffset);
            const FVector End = Proj + FVector(0, 0, TraceHeightOffset);

            FHitResult Hit;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(RepositionTraceDbg), true);
            Params.AddIgnoredActor(Pawn);

            const bool bHit = Pawn->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

            DrawDebugSphere(Pawn->GetWorld(), End, 14.0f, 12, Col, false, DebugDrawTime);

            if (bHit)
            {
                DrawDebugLine(Pawn->GetWorld(), Start, Hit.ImpactPoint, FColor::Red, false, DebugDrawTime, 0, 2.0f);
                DrawDebugLine(Pawn->GetWorld(), Hit.ImpactPoint, End, FColor::Green, false, DebugDrawTime, 0, 2.0f);
            }
            else
            {
                DrawDebugLine(Pawn->GetWorld(), Start, End, FColor::Green, false, DebugDrawTime, 0, 2.0f);
            }
        };

    if (bDebugDraw)
    {
        const FVector ObjZ = ObjectiveLoc + FVector(0, 0, TraceHeightOffset);
        const FVector CoverZ = CoverLoc + FVector(0, 0, TraceHeightOffset);

        DrawDebugSphere(Pawn->GetWorld(), CoverZ, 15.0f, 12, FColor::Cyan, false, DebugDrawTime);
        DrawDebugSphere(Pawn->GetWorld(), ObjZ, 15.0f, 12, FColor::White, false, DebugDrawTime);

        DrawDebugSphere(Pawn->GetWorld(), CandidateA + FVector(0, 0, TraceHeightOffset), 12.0f, 12, FColor::Yellow, false, DebugDrawTime);
        DrawDebugSphere(Pawn->GetWorld(), CandidateB + FVector(0, 0, TraceHeightOffset), 12.0f, 12, FColor::Yellow, false, DebugDrawTime);
    }

    bool bHasA = false;
    bool bHasB = false;

    FVector ProjA = FVector::ZeroVector;
    FVector ProjB = FVector::ZeroVector;

    struct FCandidate
    {
        bool bValid = false;
        FVector Loc = FVector::ZeroVector;
        float Score = -FLT_MAX;
    };

    FCandidate Best;

    bHasA = ProjectToNav(CandidateA, ProjA);
    if (bHasA)
    {
        const float S = ScoreCandidate(ProjA);
        if (S > Best.Score)
        {
            Best.bValid = (S > -FLT_MAX / 2.0f);
            Best.Loc = ProjA;
            Best.Score = S;
        }
    }

    bHasB = ProjectToNav(CandidateB, ProjB);
    if (bHasB)
    {
        const float S = ScoreCandidate(ProjB);
        if (S > Best.Score)
        {
            Best.bValid = (S > -FLT_MAX / 2.0f);
            Best.Loc = ProjB;
            Best.Score = S;
        }
    }

    if (bDebugDraw)
    {
        if (bHasA) { DrawTraceResult(ProjA, FColor::Blue); }
        if (bHasB) { DrawTraceResult(ProjB, FColor::Magenta); }

        if (Best.bValid)
        {
            DrawDebugSphere(Pawn->GetWorld(), Best.Loc + FVector(0, 0, TraceHeightOffset), 22.0f, 16, FColor::White, false, DebugDrawTime);
        }
    }

    BB->SetValueAsBool(HasRepositionLocationKey.SelectedKeyName, Best.bValid);
    if (Best.bValid)
    {
        BB->SetValueAsVector(RepositionLocationKey.SelectedKeyName, Best.Loc);
    }

    return EBTNodeResult::Succeeded;
}
