// Copyright Epic Games, Inc. All Rights Reserved.

#include "AICoverSystemGameMode.h"
#include "AICoverSystemCharacter.h"
#include "UObject/ConstructorHelpers.h"

AAICoverSystemGameMode::AAICoverSystemGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
