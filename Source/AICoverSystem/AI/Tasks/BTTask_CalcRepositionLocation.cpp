// Fill out your copyright notice in the Description page of Project Settings.

#include "BTTask_CalcRepositionLocation.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "GameFramework/Pawn.h"
#include "DrawDebugHelpers.h"
#include "Components/PrimitiveComponent.h"
#include "Components/CapsuleComponent.h"
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

static bool GetActorMinMaxAlongAxis(AActor* Actor, const FVector& AxisN, float& OutMinT, float& OutMaxT)
{
	if (!Actor)
	{
		return false;
	}

	const FVector N = AxisN.GetSafeNormal();
	if (N.IsNearlyZero())
	{
		return false;
	}

	bool bHasAny = false;
	float MinT = FLT_MAX;
	float MaxT = -FLT_MAX;

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
			O + FVector(E.X,  E.Y,  E.Z)
		};

		for (const FVector& P : Corners)
		{
			const float T = FVector::DotProduct(P, N);
			MinT = FMath::Min(MinT, T);
			MaxT = FMath::Max(MaxT, T);
			bHasAny = true;
		}
	}

	if (!bHasAny)
	{
		return false;
	}

	OutMinT = MinT;
	OutMaxT = MaxT;
	return true;
}

namespace
{
	struct FRepositionClaim
	{
		TWeakObjectPtr<AActor> Claimant;
		FVector Location = FVector::ZeroVector;
		float Radius = 0.0f;
		float ExpireTime = 0.0f;
	};

	static TMap<TWeakObjectPtr<AActor>, TArray<FRepositionClaim>> GRepositionClaims;

	static void CleanupRepositionClaims(const float Now)
	{
		for (auto It = GRepositionClaims.CreateIterator(); It; ++It)
		{
			const TWeakObjectPtr<AActor> Wall = It.Key();
			if (!Wall.IsValid())
			{
				It.RemoveCurrent();
				continue;
			}

			TArray<FRepositionClaim>& Claims = It.Value();
			for (int32 Idx = Claims.Num() - 1; Idx >= 0; --Idx)
			{
				const bool bExpired = (Claims[Idx].ExpireTime <= Now);
				const bool bDead = !Claims[Idx].Claimant.IsValid();
				if (bExpired || bDead)
				{
					Claims.RemoveAtSwap(Idx);
				}
			}

			if (Claims.Num() == 0)
			{
				It.RemoveCurrent();
			}
		}
	}

	static bool IsClaimedNear(AActor* WallActor, const FVector& Loc, const float Radius, AActor* Self, const float Now)
	{
		if (!WallActor)
		{
			return false;
		}

		TArray<FRepositionClaim>* ClaimsPtr = GRepositionClaims.Find(WallActor);
		if (!ClaimsPtr)
		{
			return false;
		}

		for (const FRepositionClaim& Claim : *ClaimsPtr)
		{
			if (Claim.ExpireTime <= Now)
			{
				continue;
			}

			AActor* ClaimantActor = Claim.Claimant.Get();
			if (!ClaimantActor || ClaimantActor == Self)
			{
				continue;
			}

			const float Combined = FMath::Max(0.0f, Claim.Radius + Radius);
			if (FVector::DistSquared2D(Claim.Location, Loc) <= FMath::Square(Combined))
			{
				return true;
			}
		}

		return false;
	}

	static void SetClaim(AActor* WallActor, const FVector& Loc, const float Radius, AActor* Self, const float Now, const float Duration)
	{
		if (!WallActor || !Self)
		{
			return;
		}

		TArray<FRepositionClaim>& Claims = GRepositionClaims.FindOrAdd(WallActor);

		for (int32 Idx = Claims.Num() - 1; Idx >= 0; --Idx)
		{
			if (Claims[Idx].Claimant.Get() == Self)
			{
				Claims.RemoveAtSwap(Idx);
			}
		}

		FRepositionClaim NewClaim;
		NewClaim.Claimant = Self;
		NewClaim.Location = Loc;
		NewClaim.Radius = Radius;
		NewClaim.ExpireTime = Now + FMath::Max(0.05f, Duration);

		Claims.Add(NewClaim);
	}
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

	if (DisableReposition)
	{
		return EBTNodeResult::Succeeded;
	}

	BB->SetValueAsBool(HasRepositionLocationKey.SelectedKeyName, false);
	BB->ClearValue(RepositionLocationKey.SelectedKeyName);

	const float Now = Pawn->GetWorld() ? Pawn->GetWorld()->GetTimeSeconds() : 0.0f;
	CleanupRepositionClaims(Now);

	float ClaimRadius = 60.0f;
	if (UCapsuleComponent* Capsule = Pawn->FindComponentByClass<UCapsuleComponent>())
	{
		ClaimRadius = Capsule->GetScaledCapsuleRadius() * FMath::Max(0.1f, RepositionClaimRadiusMultiplier);
	}

	AAICS_CoverPoint* Cover = Cast<AAICS_CoverPoint>(BB->GetValueAsObject(CurrentCoverKey.SelectedKeyName));
	if (!Cover)
	{
		return EBTNodeResult::Succeeded;
	}

	const FVector CoverLoc = Cover->GetActorLocation();
	const FVector CoverForward = Cover->GetActorForwardVector().GetSafeNormal2D();
	const FVector AlongCover = FVector::CrossProduct(FVector::UpVector, CoverForward).GetSafeNormal2D();

	const FVector ObjectiveLoc = BB->GetValueAsVector(ObjectiveLocationKey.SelectedKeyName);

	AActor* NextCoverActor = Cast<AActor>(BB->GetValueAsObject(NextCoverKey.SelectedKeyName));

	bool bUseNext = false;
	if (NextCoverActor)
	{
		const FVector ToNext2D = (NextCoverActor->GetActorLocation() - CoverLoc).GetSafeNormal2D();
		const FVector ToObj2D = (ObjectiveLoc - CoverLoc).GetSafeNormal2D();

		if (!ToNext2D.IsNearlyZero() && !ToObj2D.IsNearlyZero())
		{
			const float Agreement = FVector::DotProduct(ToNext2D, ToObj2D);
			bUseNext = (Agreement > -0.2f);
		}
		else
		{
			bUseNext = true;
		}
	}

	const FVector DirectionTargetLoc = (bUseNext && NextCoverActor) ? NextCoverActor->GetActorLocation() : ObjectiveLoc;

	AActor* CoverActor = nullptr;
	{
		FHitResult Hit;
		const FVector Start = CoverLoc + FVector(0, 0, HighTraceHeight);
		const FVector End = DirectionTargetLoc + FVector(0, 0, HighTraceHeight);

		FCollisionQueryParams Params(SCENE_QUERY_STAT(CalcRepositionLocation), false);
		Params.AddIgnoredActor(Pawn);
		Params.AddIgnoredActor(Cover);

		const bool bHit = Pawn->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
		if (bHit && Hit.GetActor())
		{
			CoverActor = Hit.GetActor();
		}

		if (bDebugDraw)
		{
			DrawDebugLine(Pawn->GetWorld(), Start, End, bHit ? FColor::Red : FColor::Green, false, DebugDrawTime, 0, 1.0f);
		}
	}

	if (!CoverActor)
	{
		return EBTNodeResult::Succeeded;
	}

	float MinT = 0.0f;
	float MaxT = 0.0f;
	if (!GetActorMinMaxAlongAxis(CoverActor, AlongCover, MinT, MaxT))
	{
		return EBTNodeResult::Succeeded;
	}

	const FVector AxisN = AlongCover.GetSafeNormal2D();
	const float CoverT = FVector::DotProduct(CoverLoc, AxisN);

	const FVector RawEdgePlus = CoverLoc + AxisN * (MaxT - CoverT);
	const FVector RawEdgeMinus = CoverLoc + AxisN * (MinT - CoverT);

	FVector EdgePlus = RawEdgePlus - AxisN * EdgeInset;
	FVector EdgeMinus = RawEdgeMinus + AxisN * EdgeInset;

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Pawn->GetWorld());
	if (!NavSys)
	{
		return EBTNodeResult::Succeeded;
	}

	auto IsBlockedFromObjective = [&](const FVector& TestLoc) -> bool
		{
			const FVector Start = DirectionTargetLoc + FVector(0, 0, HighTraceHeight);
			const FVector End = TestLoc + FVector(0, 0, HighTraceHeight);

			FHitResult Hit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(RepositionCoverBlock), false);
			Params.AddIgnoredActor(Pawn);

			return Pawn->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
		};

	auto IsGoodLateralMove = [&](const FVector& TestLoc, const FVector& SideDir) -> bool
		{
			UNavigationPath* Path = NavSys->FindPathToLocationSynchronously(Pawn->GetWorld(), Pawn->GetActorLocation(), TestLoc, Pawn);
			if (!Path || Path->PathPoints.Num() < 2)
			{
				return false;
			}

			const float DirectDist = FVector::Dist2D(Pawn->GetActorLocation(), TestLoc);
			const float PathDist = Path->GetPathLength();

			if (DirectDist <= KINDA_SMALL_NUMBER)
			{
				return false;
			}

			if (PathDist / DirectDist > MaxLateralDetourRatio)
			{
				return false;
			}

			const FVector FirstStep = (Path->PathPoints[1] - Path->PathPoints[0]).GetSafeNormal2D();
			const FVector SideN = SideDir.GetSafeNormal2D();

			if (FVector::DotProduct(FirstStep, SideN) < MinFirstStepDot)
			{
				return false;
			}

			return true;
		};

	auto IsGoodExitMove = [&](const FVector& TestLoc, const FVector& SideDir) -> bool
		{
			const FVector MoveDir = (TestLoc - Pawn->GetActorLocation()).GetSafeNormal2D();
			const FVector SideN = SideDir.GetSafeNormal2D();

			return FVector::DotProduct(MoveDir, SideN) > -0.5f;
		};

	const FVector PreferredSide = FVector::DotProduct((DirectionTargetLoc - CoverLoc).GetSafeNormal2D(), AxisN) >= 0.0f ? AxisN : -AxisN;

	auto ValidateAndFixEdge = [&](const FVector& InEdge, const FVector& SideDir, FVector& OutEdge) -> bool
		{
			FVector TryLoc = InEdge;

			bool bDrewClaimDebug = false;

			for (int32 TryIdx = 0; TryIdx <= MaxEdgeInwardTries; ++TryIdx)
			{
				FVector Proj;

				if (ProjectToNavLimited(NavSys, TryLoc, NavProjectExtent, MaxNavSnapDistance2D, Proj))
				{
					if (IsBlockedFromObjective(Proj))
					{
						if (!IsClaimedNear(CoverActor, Proj, ClaimRadius, Pawn, Now))
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
						else if (bDebugDraw && !bDrewClaimDebug)
						{
							DrawDebugSphere(Pawn->GetWorld(), Proj, ClaimRadius, 8, FColor::Orange, false, DebugDrawTime);
							bDrewClaimDebug = true;
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

	FVector PrefFixed;
	FVector OtherFixed;

	const bool bPrefValid = ValidateAndFixEdge(PrefRaw, PreferredSide, PrefFixed);
	const bool bOtherValid = ValidateAndFixEdge(OtherRaw, -PreferredSide, OtherFixed);

	bool bHasBest = false;
	FVector BestLoc = FVector::ZeroVector;

	if (bPrefValid)
	{
		bHasBest = true;
		BestLoc = PrefFixed;
	}
	else if (bOtherValid)
	{
		bHasBest = true;
		BestLoc = OtherFixed;
	}

	if (bDebugDraw)
	{
		DrawDebugSphere(Pawn->GetWorld(), CoverLoc, 16.0f, 8, FColor::Cyan, false, DebugDrawTime);

		if (bPrefValid)
		{
			DrawDebugSphere(Pawn->GetWorld(), PrefFixed, 18.0f, 12, FColor::Green, false, DebugDrawTime);
		}
		else
		{
			DrawDebugSphere(Pawn->GetWorld(), PrefRaw, 18.0f, 12, FColor::Red, false, DebugDrawTime);
		}

		if (bOtherValid)
		{
			DrawDebugSphere(Pawn->GetWorld(), OtherFixed, 18.0f, 12, FColor::Green, false, DebugDrawTime);
		}
		else
		{
			DrawDebugSphere(Pawn->GetWorld(), OtherRaw, 18.0f, 12, FColor::Red, false, DebugDrawTime);
		}

		if (bHasBest)
		{
			DrawDebugSphere(Pawn->GetWorld(), BestLoc, 22.0f, 12, FColor::Yellow, false, DebugDrawTime);
		}
	}

	BB->SetValueAsBool(HasRepositionLocationKey.SelectedKeyName, bHasBest);
	if (bHasBest)
	{
		if (CoverActor)
		{
			SetClaim(CoverActor, BestLoc, ClaimRadius, Pawn, Now, RepositionClaimDuration);
		}

		BB->SetValueAsVector(RepositionLocationKey.SelectedKeyName, BestLoc);
	}

	return EBTNodeResult::Succeeded;
}
