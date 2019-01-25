// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ROXBasePawn.h"
#include "ROXHUD.h"
#include "ROXTracker.h"
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Engine.h"
#include "ROXPlayerController.generated.h"

UENUM(BlueprintType)
enum class EROXHMDDeviceType : uint8
{
	EDT_OculusRift				UMETA(DisplayName = "OculusRift"),
	EDT_Morpheus				UMETA(DisplayName = "Morpheus"),
	EDT_ES2GenericStereoMesh	UMETA(DisplayName = "ES2GenericStereoMesh"),
	EDT_SteamVR					UMETA(DisplayName = "SteamVR"),
	EDT_GearVR					UMETA(DisplayName = "GearVR"),
	EDT_GoogleVR				UMETA(DisplayName = "GoogleVR")
};

/**
 * 
 */
UCLASS()
class ROBOTRIX_API AROXPlayerController : public APlayerController
{
	GENERATED_BODY()
private:
	AROXBasePawn* CachedPawn;

protected:
	bool bMoveCameraModifier;
	bool bMoveVRCameraControllersModifier;

	bool isHMDEnabled;
	EROXHMDDeviceType HMDDeviceType;

public:
	AROXPlayerController();

	void BeginPlay();
	void SetupInputComponent();

	void MoveForward(float Value);
	void MoveRight(float Value);
	void AddControllerYawInput(float Value);
	void TurnAtRate(float Value);
	void AddControllerPitchInput(float Value);
	void LookUpAtRate(float Value);
	void MoveCameraUpDown(float Value);
	void ToggleMoveCameraModifier();
	void ToggleMoveVRCamControllersModifier();
	void GraspRightHand(float Value);
	void GraspLeftHand(float Value);
	float JoystickAxisTunning(float x);

	void OnStartRecording();
	void OnResetVR();
	void OnShowCamTexture();
	void OnRestartLevel();
	void OnStartProfiling();
	void OnShowDebugging();

	// Viewmodes
	void Lit();
	void Object();
	void Depth();
	void Normal();

	// Viewtargets
	void ChangeViewTarget(AActor* ViewTarget);
	void CameraNext();
	void CameraPrev();

	// Screenshot
	void TakeScreenshot();
	void TakeDepthScreenshot();
	void SetRecordSettings();
};