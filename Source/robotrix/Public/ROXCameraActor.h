// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "ROXCameraActor.generated.h"

/**
 * 
 */
UCLASS()
class ROBOTRIX_API AROXCameraActor : public ACameraActor
{
	GENERATED_BODY()

	AROXCameraActor();	
	virtual void BeginPlay() override;
};
