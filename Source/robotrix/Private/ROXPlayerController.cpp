// Fill out your copyright notice in the Description page of Project Settings.

#include "ROXPlayerController.h"
#include "HUDInteractionInterface.h"
#include "EngineUtils.h"
#include "IXRTrackingSystem.h"
#include "IHeadMountedDisplay.h"
#include "UnrealClient.h"


AROXPlayerController::AROXPlayerController()
	: bMoveCameraModifier(false)
	, bMoveVRCameraControllersModifier(false)
	, HMDDeviceType(EROXHMDDeviceType::EDT_SteamVR)
{
}

void AROXPlayerController::BeginPlay()
{
	CachedPawn = Cast<AROXBasePawn>(this->GetPawn());
	check(IsValid(CachedPawn));

	if (CachedPawn && !CachedPawn->isRecordMode())
	{
		this->DetachFromPawn();
	}

	if (GEngine->XRSystem.IsValid())
	{
		if (GEngine->XRSystem->IsHeadTrackingAllowed())
		{
			isHMDEnabled = true;
			HMDDeviceType = ((EROXHMDDeviceType)GEngine->XRSystem->GetHMDDevice()->GetHMDDeviceType());
		}
	}
}

void AROXPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	InputComponent->BindAxis("MoveForward", this, &AROXPlayerController::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AROXPlayerController::MoveRight);
	InputComponent->BindAxis("Turn", this, &AROXPlayerController::AddControllerYawInput);
	InputComponent->BindAxis("TurnRate", this, &AROXPlayerController::TurnAtRate);
	InputComponent->BindAxis("LookUp", this, &AROXPlayerController::AddControllerPitchInput);
	InputComponent->BindAxis("LookUpRate", this, &AROXPlayerController::LookUpAtRate);
	InputComponent->BindAxis("MoveCameraUpDown", this, &AROXPlayerController::MoveCameraUpDown);
	InputComponent->BindAxis("GraspRightHand", this, &AROXPlayerController::GraspRightHand);
	InputComponent->BindAxis("GraspLeftHand", this, &AROXPlayerController::GraspLeftHand);

	InputComponent->BindAction("MoveVRCamControllersModifier", IE_Pressed, this, &AROXPlayerController::ToggleMoveVRCamControllersModifier);
	InputComponent->BindAction("MoveCamModifier", IE_Pressed, this, &AROXPlayerController::ToggleMoveCameraModifier);
	InputComponent->BindAction("StartRecording", IE_Pressed, this, &AROXPlayerController::OnStartRecording);
	InputComponent->BindAction("ResetVR", IE_Pressed, this, &AROXPlayerController::OnResetVR);
	InputComponent->BindAction("ShowCamTexture", IE_Pressed, this, &AROXPlayerController::OnShowCamTexture);
	InputComponent->BindAction("RestartLevel", IE_Pressed, this, &AROXPlayerController::OnRestartLevel);
	InputComponent->BindAction("StartProfiling", IE_Pressed, this, &AROXPlayerController::OnStartProfiling);
	InputComponent->BindAction("ShowDebugging", IE_Pressed, this, &AROXPlayerController::OnShowDebugging);

	InputComponent->BindAction("ChangeLit", IE_Pressed, this, &AROXPlayerController::Lit);
	InputComponent->BindAction("ChangeVertex", IE_Pressed, this, &AROXPlayerController::Object);
	InputComponent->BindAction("ChangeDepth", IE_Pressed, this, &AROXPlayerController::Depth);
	InputComponent->BindAction("ChangeNormal", IE_Pressed, this, &AROXPlayerController::Normal);

	InputComponent->BindAction("CameraNext", IE_Pressed, this, &AROXPlayerController::CameraNext);
	InputComponent->BindAction("CameraPrev", IE_Pressed, this, &AROXPlayerController::CameraPrev);
	InputComponent->BindAction("TakeScreenshot", IE_Pressed, this, &AROXPlayerController::TakeScreenshot);
	InputComponent->BindAction("TakeDepthScreenshot", IE_Pressed, this, &AROXPlayerController::TakeDepthScreenshot);

	InputComponent->BindAction("SetRecordSettings", IE_Pressed, this, &AROXPlayerController::SetRecordSettings);
}

void AROXPlayerController::MoveForward(float Value)
{
	float val = JoystickAxisTunning(Value);
	if (val != 0.0f)
	{
		if (!bMoveCameraModifier && !bMoveVRCameraControllersModifier)
		{
			/*
			FVector Direction = CachedPawn->GetActorForwardVector();
			CachedPawn->SetActorRelativeLocation(FVector(	CachedPawn->GetActorLocation().X + 10.f * Value * Direction.X,
													CachedPawn->GetActorLocation().Y + 10.f * Value * Direction.Y,
													CachedPawn->GetActorLocation().Z + 10.f * Value * Direction.Z ));
			*/
			CachedPawn->AddActorLocalOffset((val * GetWorld()->GetDeltaSeconds() * CachedPawn->GetSpeedModifier()) * GetActorForwardVector());

			//PlayerCameraManager->SetActorLocation(CachedPawn->GetActorLocation());
		}
		else if (bMoveVRCameraControllersModifier)
		{
			CachedPawn->MoveVRCameraControllersRelative(FVector(val * GetWorld()->GetDeltaSeconds() * 15.0f, 0.0f, 0.0f));
		}
		else if (bMoveCameraModifier)
		{
			CachedPawn->MoveCameraRelative(FVector(val * GetWorld()->GetDeltaSeconds() * 15.0f, 0.0f, 0.0f));
		}
	}
}	

void AROXPlayerController::MoveRight(float Value)
{
	float val = JoystickAxisTunning(Value);
	if (val != 0.0f)
	{
		if (!bMoveCameraModifier && !bMoveVRCameraControllersModifier)
		{
			/*
			FVector Direction = CachedPawn->GetActorRightVector();
			CachedPawn->SetActorRelativeLocation(FVector(	CachedPawn->GetActorLocation().X + 10.f * Value * Direction.X,
													CachedPawn->GetActorLocation().Y + 10.f * Value * Direction.Y,
													CachedPawn->GetActorLocation().Z + 10.f * Value * Direction.Z ));
			*/
			CachedPawn->AddActorLocalOffset((val * GetWorld()->GetDeltaSeconds() * CachedPawn->GetSpeedModifier()) * GetActorRightVector());
			//PlayerCameraManager->SetActorLocation(CachedPawn->GetActorLocation());
		}
		else if (bMoveVRCameraControllersModifier)
		{
			CachedPawn->MoveVRCameraControllersRelative(FVector(0.0f, val * GetWorld()->GetDeltaSeconds() * 15.0f, 0.0f));
		}
		else if (bMoveCameraModifier)
		{
			CachedPawn->MoveCameraRelative(FVector(0.0f, val * GetWorld()->GetDeltaSeconds() * 15.0f, 0.0f));
		}
	}
}

void AROXPlayerController::AddControllerYawInput(float Value)
{
	if (Value != 0.0f)
	{
		if (!bMoveCameraModifier)
		{
			CachedPawn->SetActorRelativeRotation(FRotator(CachedPawn->GetActorRotation().Pitch,
				CachedPawn->GetActorRotation().Yaw + Value * 1.0f,
				CachedPawn->GetActorRotation().Roll));
		}
	}
}

void AROXPlayerController::TurnAtRate(float Value)
{
	if (Value != 0.0f)
	{
		if (!bMoveCameraModifier)
		{
			CachedPawn->SetActorRelativeRotation(FRotator(CachedPawn->GetActorRotation().Pitch,
				CachedPawn->GetActorRotation().Yaw + Value * 1.0f,
				CachedPawn->GetActorRotation().Roll));
		}
	}
}

void AROXPlayerController::AddControllerPitchInput(float Value)
{
	if (Value != 0.0f)
	{
		if (!bMoveCameraModifier)
		{
			CachedPawn->CameraPitchRotation(Value * -1.0f);
		}
	}
}

void AROXPlayerController::LookUpAtRate(float Value)
{
	if (Value != 0.0f)
	{
		if (!bMoveCameraModifier)
		{
			CachedPawn->CameraPitchRotation(Value);
		}
	}
}

void AROXPlayerController::MoveCameraUpDown(float Value)
{
	if (bMoveVRCameraControllersModifier)
	{
		CachedPawn->MoveVRCameraControllersRelative(FVector(0.0f, 0.0f, Value * GetWorld()->GetDeltaSeconds() * 15.0f));
	}
	else if (bMoveCameraModifier)
	{
		CachedPawn->MoveCameraRelative(FVector(0.0f, 0.0f, Value * GetWorld()->GetDeltaSeconds() * 15.0f));
	}
}

void AROXPlayerController::GraspRightHand(float Value)
{
	CachedPawn->GraspRightHand(Value);
}

void AROXPlayerController::GraspLeftHand(float Value)
{
	CachedPawn->GraspLeftHand(Value);
}

/** Player pressed start recording action */
void AROXPlayerController::OnStartRecording()
{
	const bool bImplementsInterface(GetHUD() && GetHUD()->Implements<UHUDInteractionInterface>());

	if(CachedPawn->GetTracker()) { 
		CachedPawn->GetTracker()->ToggleRecording();
		if (bImplementsInterface) {
			IHUDInteractionInterface::Execute_ToggleRecording(GetHUD());
		}
	}
	else if (bImplementsInterface) {
		IHUDInteractionInterface::Execute_HudShowError(GetHUD(), "A Tracker actor must be added to the scene and selected in pawn details.");
	}

}

/** Player pressed Show Camera Texture action */
void AROXPlayerController::OnShowCamTexture()
{
	if (GetHUD() && GetHUD()->Implements<UHUDInteractionInterface>())
		IHUDInteractionInterface::Execute_ToggleCamTexture(GetHUD());
}

/** Player pressed Reset VR action */
void AROXPlayerController::OnResetVR()
{
	
	if (HMDDeviceType == EROXHMDDeviceType::EDT_OculusRift)
	{
		//ConsoleCommand("vr.TrackingOrigin 0");
		UE_LOG(LogTemp, Warning, TEXT("Oculus Rift"));
	}
	else if (HMDDeviceType == EROXHMDDeviceType::EDT_SteamVR)
	{
		UE_LOG(LogTemp, Warning, TEXT("HTC Vive"));
	}

	ConsoleCommand("vr.HeadTracking.Reset");
	if (GetHUD() && GetHUD()->Implements<UHUDInteractionInterface>())
		IHUDInteractionInterface::Execute_HudResetVR(GetHUD());
}

/** Player pressed Reset Level action */
void AROXPlayerController::OnRestartLevel()
{
	RestartLevel();
}

/** Player pressed start/stop profiling action */
void AROXPlayerController::OnStartProfiling()
{
	if (GetHUD() && GetHUD()->Implements<UHUDInteractionInterface>())
		IHUDInteractionInterface::Execute_ToggleProfiling(GetHUD());
}

/** Player pressed enable/disable debugging action */
void AROXPlayerController::OnShowDebugging()
{
	if (GetHUD() && GetHUD()->Implements<UHUDInteractionInterface>())
		IHUDInteractionInterface::Execute_ToggleDebugging(GetHUD());
}

void AROXPlayerController::ToggleMoveCameraModifier()
{
	if (!bMoveCameraModifier)
	{
		bMoveCameraModifier = true;
		bMoveVRCameraControllersModifier = false;
	}
	else
	{
		bMoveCameraModifier = false;
	}
	
	
	if (GetHUD() && GetHUD()->Implements<UHUDInteractionInterface>())
	{
		if (bMoveCameraModifier)
			IHUDInteractionInterface::Execute_HudShowState(GetHUD(), "Move camera enabled", 5.0f);
		else
			IHUDInteractionInterface::Execute_HudShowState(GetHUD(), "Move camera disabled", 5.0f);
	}
}

void AROXPlayerController::ToggleMoveVRCamControllersModifier()
{
	if (!bMoveVRCameraControllersModifier)
	{
		bMoveVRCameraControllersModifier = true;
		bMoveCameraModifier = false;
	}
	else
	{
		bMoveVRCameraControllersModifier = false;
	}

	if (GetHUD() && GetHUD()->Implements<UHUDInteractionInterface>())
	{
		if (bMoveVRCameraControllersModifier)
			IHUDInteractionInterface::Execute_HudShowState(GetHUD(), "Move camera and motion controllers enabled", 5.0f);
		else
			IHUDInteractionInterface::Execute_HudShowState(GetHUD(), "Move camera and motion controllers disabled", 5.0f);
	}
}

// Viewmodes, Viewtargets and Screenshots -----------------------------

void AROXPlayerController::Lit()
{
	CachedPawn->GetTracker()->Lit();
}

void AROXPlayerController::Object()
{
	CachedPawn->GetTracker()->Object();
}

void AROXPlayerController::Depth()
{
	CachedPawn->GetTracker()->Depth();
}

void AROXPlayerController::Normal()
{
	CachedPawn->GetTracker()->Normal();
}

void AROXPlayerController::ChangeViewTarget(AActor* ViewTarget)
{
	SetViewTarget(ViewTarget);
}

void AROXPlayerController::CameraNext()
{
	SetViewTarget(CachedPawn->GetTracker()->CameraNext());
}

void AROXPlayerController::CameraPrev()
{
	SetViewTarget(CachedPawn->GetTracker()->CameraPrev());
}

void AROXPlayerController::TakeScreenshot()
{
	CachedPawn->GetTracker()->TakeScreenshot();
}

void AROXPlayerController::TakeDepthScreenshot()
{
	CachedPawn->GetTracker()->TakeScreenshot(EROXViewMode::RVM_Depth);
}

void AROXPlayerController::SetRecordSettings()
{
	CachedPawn->GetTracker()->SetRecordSettings();
	ConsoleCommand("viewmode unlit");
	ConsoleCommand("stat fps");
	ConsoleCommand("t.MaxFps 60");
}

float AROXPlayerController::JoystickAxisTunning(float x)
{
	// Polynomial functions for which f(-1) = -1, f(0) = 0 y f(1) = 1
	// They grow slower when x is near to 0, and faster when approaching to 1
	//return x; // Lineal
	return 0.8*x*x*x*x*x + 0.4*x*x*x + 0.04*x; // Smoother
	//return 1.126*x*x*x*x*x - 0.165*x*x*x + 0.039*x; // Harsher
}