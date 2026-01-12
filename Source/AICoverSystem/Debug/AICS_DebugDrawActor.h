#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AICS_DebugDrawActor.generated.h"

UCLASS()
class AICOVERSYSTEM_API AAICS_DebugDrawActor : public AActor
{
	GENERATED_BODY()

public:
	AAICS_DebugDrawActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

protected:
	void DrawDebugNow();

protected:
	UPROPERTY(EditAnywhere, Category = "debug")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, Category = "debug")
	float DrawInterval = 0.15f;

	UPROPERTY(EditAnywhere, Category = "debug")
	float DrawDuration = 0.2f;

	UPROPERTY(EditAnywhere, Category = "debug")
	bool bDrawCoverPoints = true;

	UPROPERTY(EditAnywhere, Category = "debug")
	bool bDrawAIState = true;

	UPROPERTY(EditAnywhere, Category = "debug")
	bool bDrawObjective = true;

	UPROPERTY(EditAnywhere, Category = "debug|blackboard")
	FName ObjectiveLocationKeyName = TEXT("ObjectiveLocation");

	UPROPERTY(EditAnywhere, Category = "debug|blackboard")
	FName CurrentCoverKeyName = TEXT("CurrentCover");

	UPROPERTY(EditAnywhere, Category = "debug|blackboard")
	FName TargetCoverKeyName = TEXT("TargetCover");

	UPROPERTY(EditAnywhere, Category = "debug|blackboard")
	FName NextCoverKeyName = TEXT("NextCover");

	UPROPERTY(EditAnywhere, Category = "debug|blackboard")
	FName HasRepositionLocationKeyName = TEXT("HasRepositionLocation");

	UPROPERTY(EditAnywhere, Category = "debug|blackboard")
	FName RepositionLocationKeyName = TEXT("RepositionLocation");

	UPROPERTY(EditAnywhere, Category = "debug|blackboard")
	FName InCoverKeyName = TEXT("InCover");

	UPROPERTY(EditAnywhere, Category = "debug|visual")
	float CoverArrowLength = 60.0f;

	UPROPERTY(EditAnywhere, Category = "debug|visual")
	float CoverTextHeight = 35.0f;

	UPROPERTY(EditAnywhere, Category = "debug|visual")
	float AITextHeight = 95.0f;

	UPROPERTY(EditAnywhere, Category = "debug|visual")
	float RepositionSphereRadius = 18.0f;

private:
	float TimeAccum = 0.0f;
};
