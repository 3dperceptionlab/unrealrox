// Fill out your copyright notice in the Description page of Project Settings.

#include "ROXGameMode.h"
#include "ROXPlayerController.h"
#include "ROXHUD.h"
#include "ROXBasePawn.h"


AROXGameMode::AROXGameMode(const class FObjectInitializer&ObjectInitializer)
	: Super(ObjectInitializer)
{
	PlayerControllerClass = AROXPlayerController::StaticClass();
	HUDClass = AROXHUD::StaticClass();
	DefaultPawnClass = AROXBasePawn::StaticClass();
}

