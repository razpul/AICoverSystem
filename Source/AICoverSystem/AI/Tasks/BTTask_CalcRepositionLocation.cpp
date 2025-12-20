// Fill out your copyright notice in the Description page of Project Settings.

#include "BTTask_CalcRepositionLocation.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "GameFramework/Pawn.h"
#include "DrawDebugHelpers.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"

#include <cfloat>

#include "AICoverSystem/AI/CoverQuery/AICS_CoverPoint.h"

UBTTask_CalcRepositionLocation::UBTTask_CalcRepositionLocation()
{
    NodeName = "Calc Reposition Location";
}

static bool ProjectToNavLimited(UNavigationSystemV1* NavSys, const FVector& InLoc, float ExtentSize, float MaxSnap2D, FVector& OutLoc)
{
    if (!NavSys)
    {
        return false;
    }

    FNavLocation Projected;
    const FVector Extent(ExtentSize, ExtentSize, ExtentSize);

    if (!NavSys->ProjectPointToNavigation(InLoc, Projected, Extent))
    {
        return false;
    }

    OutLoc = Projected.Location;

    if (FVector::Dist2D(OutLoc, InLoc) > MaxSnap2D)
    {
        return false;
    }

    return true;
}

static bool GetActorMinMaxAlongAxis(AActor* Actor, const FVector& Axis, float& OutMin, float& OutMax)
{
    if (!Actor)
    {
        return false;
    }

    const FVector N = Axis.GetSafeNormal();
    bool bHasAny = false;

    OutMin = FLT_MAX;
    OutMax = -FLT_MAX;

    TInlineComponentArray<UPrimitiveComponent*> PrimComps;
    Actor->GetComponents(PrimComps);

    for (UPrimitiveComponent* Prim : PrimComps)
    {
        if (!Prim || !Prim->IsRegistered())
        {
            continue;
        }

        if (!Prim->IsCollisionEnabled())
        {
            continue;
        }

        if (Prim->GetCollisionResponseToChannel(ECC_Visibility) != ECR_Block)
        {
            continue;
        }

        if (UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Prim))
        {
            FVector LMin, LMax;
            SMC->GetLocalBounds(LMin, LMax);

            const FTransform& Xf = SMC->GetComponentTransform();

            const FVector Corners[8] =
            {
                Xf.TransformPosition(FVector(LMin.X, LMin.Y, LMin.Z)),
                Xf.TransformPosition(FVector(LMin.X, LMin.Y, LMax.Z)),
                Xf.TransformPosition(FVector(LMin.X, LMax.Y, LMin.Z)),
                Xf.TransformPosition(FVector(LMin.X, LMax.Y, LMax.Z)),
                Xf.TransformPosition(FVector(LMax.X, LMin.Y, LMin.Z)),
                Xf.TransformPosition(FVector(LMax.X, LMin.Y, LMax.Z)),
                Xf.TransformPosition(FVector(LMax.X, LMax.Y, LMin.Z)),
                Xf.TransformPosition(FVector(LMax.X, LMax.Y, LMax.Z)),
            };

            for (const FVector& P : Corners)
            {
                const float T = FVector::DotProduct(P, N);
                OutMin = FMath::Min(OutMin, T);
                OutMax = FMath::Max(OutMax, T);
                bHasAny = true;
            }

            continue;
        }

        const FVector O = Prim->Bounds.Origin;
        const FVector E = Prim->Bounds.BoxExtent;

        const FVector Corners[8] =
        {
            O + FVector(-E.X, -E.Y, -E.Z),
            O + FVector(-E.X, -E.Y,  E.Z),
            O + FVector(-E.X,  E.Y, -E.Z),
            O + FVector(-E.X,  E.Y,  E.Z),
            O + FVector(E.X, -E.Y, -E.Z),
            O + FVector(E.X, -E.Y,  E.Z),
            O + FVector(E.X,  E.Y, -E.Z),
            O + FVector(E.X,  E.Y,  E.Z),
        };

        for (const FVector& P : Corners)
        {
            const float T = FVector::DotProduct(P, N);
            OutMin = FMath::Min(OutMin, T);
            OutMax = FMath::Max(OutMax, T);
            bHasAny = true;
        }
    }

    return bHasAny;
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

    const AActor* NextCoverActor = Cast<AActor>(BB->GetValueAsObject(NextCoverKey.SelectedKeyName));
    const FVector DirectionTargetLoc = NextCoverActor ? NextCoverActor->GetActorLocation() : ObjectiveLoc;

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

    const FVector ToTarget = (DirectionTargetLoc - CoverLoc).GetSafeNormal2D();
    const float SideDot = FVector::DotProduct(AlongCover, ToTarget);

    const FVector PreferredSide = (SideDot >= 0.0f) ? AlongCover : -AlongCover;
    const FVector OtherSide = -PreferredSide;

    const float ThreatHeight = HighTraceHeight;
    const float TargetHeight = (CurrentCover->CoverType == ECoverType::Low) ? LowTraceHeight : HighTraceHeight;

    auto TraceToObjectiveGetHit = [&](FHitResult& OutHit) -> bool
        {
            const FVector Start = CoverLoc + FVector(0, 0, TargetHeight);
            const FVector End = ObjectiveLoc + FVector(0, 0, ThreatHeight);

            FCollisionQueryParams Params(SCENE_QUERY_STAT(RepositionCoverPick), false);
            Params.AddIgnoredActor(Pawn);
            Params.AddIgnoredActor(CurrentCover);

            return Pawn->GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, Params);
        };

    auto IsBlockedFromObjective = [&](const FVector& TestLoc) -> bool
        {
            const FVector Start = ObjectiveLoc + FVector(0, 0, ThreatHeight);
            const FVector End = TestLoc + FVector(0, 0, TargetHeight);

            FHitResult Hit;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(RepositionTrace), false);
            Params.AddIgnoredActor(Pawn);

            return Pawn->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
        };

    auto IsGoodLateralMove = [&](const FVector& Dest, const FVector& SideDir) -> bool
        {
            FVector StartLoc = Pawn->GetActorLocation();

            {
                FNavLocation StartProjected;
                const FVector Extent(NavProjectExtent, NavProjectExtent, NavProjectExtent);
                if (NavSys->ProjectPointToNavigation(StartLoc, StartProjected, Extent))
                {
                    StartLoc = StartProjected.Location;
                }
            }

            UNavigationPath* Path = NavSys->FindPathToLocationSynchronously(Pawn->GetWorld(), StartLoc, Dest, Pawn);
            if (!Path || !Path->IsValid() || Path->IsPartial())
            {
                return false;
            }

            const TArray<FVector>& Pts = Path->PathPoints;
            if (Pts.Num() < 2)
            {
                return true;
            }

            float PathLen2D = 0.0f;
            for (int32 i = 1; i < Pts.Num(); ++i)
            {
                PathLen2D += FVector::Dist2D(Pts[i - 1], Pts[i]);
            }

            const float Straight2D = FVector::Dist2D(StartLoc, Dest);
            if (Straight2D < 1.0f)
            {
                return true;
            }

            if (PathLen2D > Straight2D * MaxLateralDetourRatio)
            {
                return false;
            }

            const FVector FirstDir = (Pts[1] - Pts[0]).GetSafeNormal2D();
            if (FVector::DotProduct(FirstDir, SideDir.GetSafeNormal2D()) < MinFirstStepDot)
            {
                return false;
            }

            return true;
        };

    FHitResult CoverHit;
    AActor* CoverActor = nullptr;
    if (TraceToObjectiveGetHit(CoverHit))
    {
        CoverActor = CoverHit.GetActor();
    }

    if (!CoverActor)
    {
        BB->SetValueAsBool(HasRepositionLocationKey.SelectedKeyName, false);
        return EBTNodeResult::Succeeded;
    }

    float MinT = 0.0f;
    float MaxT = 0.0f;
    if (!GetActorMinMaxAlongAxis(CoverActor, AlongCover, MinT, MaxT))
    {
        BB->SetValueAsBool(HasRepositionLocationKey.SelectedKeyName, false);
        return EBTNodeResult::Succeeded;
    }

    const FVector AxisN = AlongCover.GetSafeNormal();
    const float CoverT = FVector::DotProduct(CoverLoc, AxisN);

    FVector RawEdgePlus = CoverLoc + AxisN * (MaxT - CoverT);
    FVector RawEdgeMinus = CoverLoc + AxisN * (MinT - CoverT);

    FVector EdgePlus = RawEdgePlus - AxisN * EdgeInset;
    FVector EdgeMinus = RawEdgeMinus + AxisN * EdgeInset;

    auto ValidateAndFixEdge = [&](const FVector& InEdge, const FVector& SideDir, FVector& OutEdge) -> bool
        {
            FVector TryLoc = InEdge;

            for (int32 TryIdx = 0; TryIdx <= MaxEdgeInwardTries; ++TryIdx)
            {
                FVector Proj;
                if (ProjectToNavLimited(NavSys, TryLoc, NavProjectExtent, MaxNavSnapDistance2D, Proj))
                {
                    if (IsBlockedFromObjective(Proj))
                    {
                        if (IsGoodLateralMove(Proj, SideDir))
                        {
                            OutEdge = Proj;
                            return true;
                        }
                    }
                }

                TryLoc = TryLoc - SideDir.GetSafeNormal2D() * EdgeInwardStep;
            }

            return false;
        };

    const bool bPlusIsPreferred = FVector::DotProduct(AxisN, PreferredSide) > 0.0f;

    const FVector PrefRaw = bPlusIsPreferred ? EdgePlus : EdgeMinus;
    const FVector OtherRaw = bPlusIsPreferred ? EdgeMinus : EdgePlus;

    FVector EdgePref = FVector::ZeroVector;
    FVector EdgeOther = FVector::ZeroVector;

    const bool bHasPref = ValidateAndFixEdge(PrefRaw, PreferredSide, EdgePref);
    const bool bHasOther = ValidateAndFixEdge(OtherRaw, OtherSide, EdgeOther);

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

    if (bDebugDraw)
    {
        const FVector CoverZ = CoverLoc + FVector(0, 0, TargetHeight);
        DrawDebugSphere(Pawn->GetWorld(), CoverZ, 18.0f, 12, FColor::Cyan, false, DebugDrawTime);

        if (CoverActor)
        {
            const FVector CA = CoverActor->GetActorLocation() + FVector(0, 0, TargetHeight);
            DrawDebugSphere(Pawn->GetWorld(), CA, 16.0f, 12, FColor::Orange, false, DebugDrawTime);
        }

        if (NextCoverActor)
        {
            DrawDebugSphere(Pawn->GetWorld(), DirectionTargetLoc + FVector(0, 0, TargetHeight), 18.0f, 12, FColor::Yellow, false, DebugDrawTime);
            DrawDebugLine(Pawn->GetWorld(), CoverZ, DirectionTargetLoc + FVector(0, 0, TargetHeight), FColor::Yellow, false, DebugDrawTime, 0, 1.5f);
        }

        DrawDebugSphere(Pawn->GetWorld(), EdgePlus + FVector(0, 0, TargetHeight), 12.0f, 12, FColor::Silver, false, DebugDrawTime);
        DrawDebugSphere(Pawn->GetWorld(), EdgeMinus + FVector(0, 0, TargetHeight), 12.0f, 12, FColor::Silver, false, DebugDrawTime);

        if (bHasPref)
        {
            DrawDebugSphere(Pawn->GetWorld(), EdgePref + FVector(0, 0, TargetHeight), 14.0f, 12, FColor::Blue, false, DebugDrawTime);
        }
        if (bHasOther)
        {
            DrawDebugSphere(Pawn->GetWorld(), EdgeOther + FVector(0, 0, TargetHeight), 14.0f, 12, FColor::Magenta, false, DebugDrawTime);
        }

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
