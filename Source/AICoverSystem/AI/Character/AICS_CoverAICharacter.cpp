// Fill out your copyright notice in the Description page of Project Settings.


#include "AICS_CoverAICharacter.h"

#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
AAICS_CoverAICharacter::AAICS_CoverAICharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	AIControllerClass = AAICS_CoverAICharacter::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->GetNavAgentPropertiesRef().bCanCrouch = true;
		MoveComp->MaxWalkSpeedCrouched = 200.f;
	}

}



