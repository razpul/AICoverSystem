#include "AICS_DebugDrawActor.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"

#include "AICoverSystem/AI/Character/AICS_CoverAICharacter.h"
#include "AICoverSystem/AI/CoverQuery/AICS_CoverPoint.h"

AAICS_DebugDrawActor::AAICS_DebugDrawActor()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AAICS_DebugDrawActor::BeginPlay()
{
	Super::BeginPlay();
	TimeAccum = 0.0f;
}

void AAICS_DebugDrawActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bEnabled)
	{
		return;
	}

	TimeAccum += DeltaSeconds;
	if (TimeAccum < DrawInterval)
	{
		return;
	}

	TimeAccum = 0.0f;
	DrawDebugNow();
}

void AAICS_DebugDrawActor::DrawDebugNow()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (bDrawCoverPoints)
	{
		for (TActorIterator<AAICS_CoverPoint> It(World); It; ++It)
		{
			AAICS_CoverPoint* CP = *It;
			if (!CP)
			{
				continue;
			}

			const FVector L = CP->GetActorLocation();

			const FVector Facing = CP->GetFacingDirection().GetSafeNormal2D();
			const FVector End = L + Facing * CoverArrowLength;

			const bool bReserved = CP->IsReserved();

			const FColor ArrowColor = bReserved ? FColor::Red : FColor::Green;

			DrawDebugDirectionalArrow(World, L, End, 20.0f, ArrowColor, false, DrawDuration, 0, 2.0f);

			FString TypeStr = TEXT("cover");
			if (CP->CoverType == ECoverType::Low)
			{
				TypeStr = TEXT("low");
			}
			else if (CP->CoverType == ECoverType::High)
			{
				TypeStr = TEXT("high");
			}

			const FString Text = FString::Printf(TEXT("%s %s"), *TypeStr, bReserved ? TEXT("reserved") : TEXT("free"));
			DrawDebugString(World, L + FVector(0, 0, CoverTextHeight), Text, nullptr, ArrowColor, DrawDuration, true);
		}
	}

	if (bDrawAIState)
	{
		for (TActorIterator<AAICS_CoverAICharacter> It(World); It; ++It)
		{
			AAICS_CoverAICharacter* AIPawn = *It;
			if (!AIPawn)
			{
				continue;
			}

			AAIController* AIC = Cast<AAIController>(AIPawn->GetController());
			UBlackboardComponent* BB = AIC ? AIC->GetBlackboardComponent() : nullptr;
			if (!BB)
			{
				continue;
			}

			const bool bInCover = !InCoverKeyName.IsNone() ? BB->GetValueAsBool(InCoverKeyName) : false;

			UObject* CurrentCoverObj = !CurrentCoverKeyName.IsNone() ? BB->GetValueAsObject(CurrentCoverKeyName) : nullptr;
			UObject* TargetCoverObj = !TargetCoverKeyName.IsNone() ? BB->GetValueAsObject(TargetCoverKeyName) : nullptr;
			UObject* NextCoverObj = !NextCoverKeyName.IsNone() ? BB->GetValueAsObject(NextCoverKeyName) : nullptr;

			const FString CurrName = GetNameSafe(CurrentCoverObj);
			const FString TgtName = GetNameSafe(TargetCoverObj);
			const FString NextName = GetNameSafe(NextCoverObj);

			const bool bHasRepo = !HasRepositionLocationKeyName.IsNone() ? BB->GetValueAsBool(HasRepositionLocationKeyName) : false;
			const FVector RepoLoc = !RepositionLocationKeyName.IsNone() ? BB->GetValueAsVector(RepositionLocationKeyName) : FVector::ZeroVector;

			const FVector L = AIPawn->GetActorLocation();

			const FColor LabelColor = bInCover ? FColor::Cyan : FColor::White;

			const FString Text = FString::Printf(
				TEXT("incover:%s\ncurr:%s\ntgt:%s\nnext:%s\nrepo:%s"),
				bInCover ? TEXT("1") : TEXT("0"),
				*CurrName,
				*TgtName,
				*NextName,
				bHasRepo ? TEXT("1") : TEXT("0")
			);

			DrawDebugString(World, L + FVector(0, 0, AITextHeight), Text, nullptr, LabelColor, DrawDuration, true);

			if (bHasRepo)
			{
				DrawDebugSphere(World, RepoLoc, RepositionSphereRadius, 12, FColor::Yellow, false, DrawDuration, 0, 1.5f);
				DrawDebugLine(World, L + FVector(0, 0, 60.0f), RepoLoc, FColor::Yellow, false, DrawDuration, 0, 1.0f);
			}

			if (bDrawObjective && !ObjectiveLocationKeyName.IsNone())
			{
				const FVector Obj = BB->GetValueAsVector(ObjectiveLocationKeyName);
				DrawDebugSphere(World, Obj, 22.0f, 12, FColor::Green, false, DrawDuration, 0, 1.5f);
			}
		}
	}
}
