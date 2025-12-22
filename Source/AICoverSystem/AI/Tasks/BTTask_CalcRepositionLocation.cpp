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

// helper :  project a point onto navmesh and reject if it snaps too far away
static bool ProjectToNavLimited(UNavigationSystemV1* NavSys, const FVector& InLoc, float ExtentSize, float MaxSnap2D, FVector& OutLoc)
{
	if (!NavSys)
	{
		return false;
	}

	// project location onto nav within a search box
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
// helper: find min/max extent of actor along given axis
static bool GetActorMinMaxAlongAxis(AActor* Actor, const FVector& Axis, float& OutMin, float& OutMax)
{
	if (!Actor)
	{
		return false;
	}

	// normalise axis
	const FVector N = Axis.GetSafeNormal();
	bool bHasAny = false;

	// start large
	OutMin = FLT_MAX;
	OutMax = -FLT_MAX;

	// get all primitive comps on actor
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

		// use comps that block visibility
		if (Prim->GetCollisionResponseToChannel(ECC_Visibility) != ECR_Block)
		{
			continue;
		}

		// if static mesh, use local bound and transform 8 corners into world space
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

			// project each corner on axis and update min/max
			for (const FVector& P : Corners)
			{
				const float T = FVector::DotProduct(P, N);
				OutMin = FMath::Min(OutMin, T);
				OutMax = FMath::Max(OutMax, T);
				bHasAny = true;
			}

			continue;
		}

		// fallback for non static mesh
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

		// project each corner onto axis and update min/max
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
	// get refs
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;

	if (!BB || !AIC || !Pawn)
	{
		return EBTNodeResult::Failed;
	}

	// get current cover point
	AAICS_CoverPoint* CurrentCover = Cast<AAICS_CoverPoint>(BB->GetValueAsObject(CurrentCoverKey.SelectedKeyName));
	if (!CurrentCover)
	{
		BB->SetValueAsBool(HasRepositionLocationKey.SelectedKeyName, false);
		return EBTNodeResult::Succeeded;
	}

	// decide what direction bias we want for next movement (cp or obj)
	const FVector ObjectiveLoc = BB->GetValueAsVector(ObjectiveLocationKey.SelectedKeyName);
	const FVector CoverLoc = CurrentCover->GetActorLocation();

	const AActor* NextCoverActor = Cast<AActor>(BB->GetValueAsObject(NextCoverKey.SelectedKeyName));
	const FVector DirectionTargetLoc = NextCoverActor ? NextCoverActor->GetActorLocation() : ObjectiveLoc;



	// get nav system
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Pawn->GetWorld());
	if (!NavSys)
	{
		BB->SetValueAsBool(HasRepositionLocationKey.SelectedKeyName, false);
		return EBTNodeResult::Succeeded;
	}

	FVector ExitTargetLoc = DirectionTargetLoc;

	// try to snap exit target to navmesh so path checks dont fail for dumb reasons
	{
		FNavLocation ExitProjected;
		const FVector Extent(NavProjectExtent, NavProjectExtent, NavProjectExtent);
		if (NavSys->ProjectPointToNavigation(ExitTargetLoc, ExitProjected, Extent))
		{
			ExitTargetLoc = ExitProjected.Location;
		}
	}

	// calc sideways dir along cover wall using cover facing arrow
	const FVector Facing = CurrentCover->GetFacingDirection().GetSafeNormal2D();
	FVector AlongCover = FVector::CrossProduct(FVector::UpVector, Facing).GetSafeNormal2D();
	if (AlongCover.IsNearlyZero())
	{
		AlongCover = Pawn->GetActorRightVector().GetSafeNormal2D();
	}

	// calc/decide preferred side left v right
	const FVector ToTarget = (DirectionTargetLoc - CoverLoc).GetSafeNormal2D();
	const float SideDot = FVector::DotProduct(AlongCover, ToTarget);

	const FVector PreferredSide = (SideDot >= 0.0f) ? AlongCover : -AlongCover;
	const FVector OtherSide = -PreferredSide;

	// decide trace heighs based on cover types (high/low)
	const float ThreatHeight = HighTraceHeight;
	const float TargetHeight = (CurrentCover->CoverType == ECoverType::Low) ? LowTraceHeight : HighTraceHeight;

	// HELPER lambda functions
	// find the cover mesh between cover point and objective
	auto TraceToObjectiveGetHit = [&](FHitResult& OutHit) -> bool
		{
			const FVector Start = CoverLoc + FVector(0, 0, TargetHeight);
			const FVector End = ObjectiveLoc + FVector(0, 0, ThreatHeight);

			FCollisionQueryParams Params(SCENE_QUERY_STAT(RepositionCoverPick), false);
			Params.AddIgnoredActor(Pawn);
			Params.AddIgnoredActor(CurrentCover);

			return Pawn->GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, Params);
		};

	// checks if the repos location is behind cover or not
	auto IsBlockedFromObjective = [&](const FVector& TestLoc) -> bool
		{
			const FVector Start = ObjectiveLoc + FVector(0, 0, ThreatHeight);
			const FVector End = TestLoc + FVector(0, 0, TargetHeight);

			FHitResult Hit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(RepositionTrace), false);
			Params.AddIgnoredActor(Pawn);

			return Pawn->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
		};

	// checks if moving to edge causes weird nav pathing
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

	// checks if this edge is a good launch point to move toward the next cover / objective
	// checks if leaving from this edge toward the next cover would cause a backtrack / big detour
	auto IsGoodExitMove = [&](const FVector& FromEdge, const FVector& SideDir) -> bool
		{
			UNavigationPath* Path = NavSys->FindPathToLocationSynchronously(Pawn->GetWorld(), FromEdge, ExitTargetLoc, Pawn);
			if (!Path || !Path->IsValid() || Path->IsPartial())
			{
				return false;
			}

			const TArray<FVector>& Pts = Path->PathPoints;
			if (Pts.Num() < 2)
			{
				return true;
			}

			// reject if the first step immediately goes against the side we want 
			const FVector FirstDir = (Pts[1] - Pts[0]).GetSafeNormal2D();
			if (FVector::DotProduct(FirstDir, SideDir.GetSafeNormal2D()) < -0.15f)
			{
				return false;
			}

			// reject if the path is way longer than a straight line 
			float PathLen2D = 0.0f;
			for (int32 i = 1; i < Pts.Num(); ++i)
			{
				PathLen2D += FVector::Dist2D(Pts[i - 1], Pts[i]);
			}

			const float Straight2D = FVector::Dist2D(FromEdge, ExitTargetLoc);
			if (Straight2D > 1.0f && PathLen2D > Straight2D * 1.5f)
			{
				return false;
			}

			return true;
		};


	FHitResult CoverHit;
	AActor* CoverActor = nullptr;
	// get cover mesh actor
	if (TraceToObjectiveGetHit(CoverHit))
	{
		CoverActor = CoverHit.GetActor();
	}

	if (!CoverActor)
	{
		BB->SetValueAsBool(HasRepositionLocationKey.SelectedKeyName, false);
		return EBTNodeResult::Succeeded;
	}

	// get cover actor mesh extents along wall axis
	float MinT = 0.0f;
	float MaxT = 0.0f;
	if (!GetActorMinMaxAlongAxis(CoverActor, AlongCover, MinT, MaxT))
	{
		BB->SetValueAsBool(HasRepositionLocationKey.SelectedKeyName, false);
		return EBTNodeResult::Succeeded;
	}

	// convert min/max values to edge positions
	const FVector AxisN = AlongCover.GetSafeNormal();
	const float CoverT = FVector::DotProduct(CoverLoc, AxisN);

	// calc point on actor at positive edge along axis
	FVector RawEdgePlus = CoverLoc + AxisN * (MaxT - CoverT);
	// calc point on actor at negative edge alon axis
	FVector RawEdgeMinus = CoverLoc + AxisN * (MinT - CoverT);
	// add edge inset so we stand inside more
	FVector EdgePlus = RawEdgePlus - AxisN * EdgeInset;
	FVector EdgeMinus = RawEdgeMinus + AxisN * EdgeInset;

	auto ValidateAndFixEdge = [&](const FVector& InEdge, const FVector& SideDir, FVector& OutEdge) -> bool
		{
			FVector TryLoc = InEdge;

			for (int32 TryIdx = 0; TryIdx <= MaxEdgeInwardTries; ++TryIdx)
			{
				FVector Proj;
				// project to navmesh
				if (ProjectToNavLimited(NavSys, TryLoc, NavProjectExtent, MaxNavSnapDistance2D, Proj))
				{
					if (IsBlockedFromObjective(Proj))
					{
						if (IsGoodLateralMove(Proj, SideDir))
						{
							if (IsGoodExitMove(Proj, SideDir))
							{
								OutEdge = Proj;
								return true;
							}
						}
					}
				}

				TryLoc = TryLoc - SideDir.GetSafeNormal2D() * EdgeInwardStep;
			}

			return false;
		};

	// decide raw edge side left right
	const bool bPlusIsPreferred = FVector::DotProduct(AxisN, PreferredSide) > 0.0f;

	const FVector PrefRaw = bPlusIsPreferred ? EdgePlus : EdgeMinus;
	const FVector OtherRaw = bPlusIsPreferred ? EdgeMinus : EdgePlus;

	FVector EdgePref = FVector::ZeroVector;
	FVector EdgeOther = FVector::ZeroVector;

	// validate both edges
	const bool bHasPref = ValidateAndFixEdge(PrefRaw, PreferredSide, EdgePref);
	const bool bHasOther = ValidateAndFixEdge(OtherRaw, OtherSide, EdgeOther);

	// choose best repos loc
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

	// debug draw 
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
