// Fill out your copyright notice in the Description page of Project Settings.

#include "ROXHUD.h"
#include "Engine/Canvas.h"
#include "Engine.h"
#include "IXRTrackingSystem.h"
#include "ConstructorHelpers.h"

AROXHUD::AROXHUD() :
	showCamInHUD(false),
	isRecording(false),
	isProfiling(false),
	isDebugging(false),
	stateString(""),
	stateStringRemainingTime(0.0f),
	errorString(""),
	errorStringRemainingTime(0.0f),
	ScreenPosFactor_X(2.f),
	ScreenPosFactor_Y(35.f)
{
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TextureCamObj(TEXT("Material'/Game/Common/RT_ThirdPersonCamera_Mat.RT_ThirdPersonCamera_Mat'"));
	M_TextureCam = TextureCamObj.Object;
}

void AROXHUD::BeginPlay()
{
	Super::BeginPlay();
	if (GEngine->XRSystem.IsValid() && GEngine->XRSystem->IsHeadTrackingAllowed()) {
		ScreenPosFactor_Y = 2.5f;
	}
}

void AROXHUD::DrawHUD()
{
	DrawCameraTexture(Canvas->SizeX, Canvas->SizeY);

	DrawText(isRecording ? "RECORDING" : "NOT RECORDING", isRecording ? FLinearColor::Red : FLinearColor::Black, Canvas->SizeX / ScreenPosFactor_X, Canvas->SizeY / ScreenPosFactor_Y);
	DrawStateString(Canvas->SizeX, Canvas->SizeY);
	DrawErrorString(Canvas->SizeX, Canvas->SizeY);

	Super::DrawHUD();
}

void AROXHUD::DrawCameraTexture(int32 SizeX, int32 SizeY)
{
	if (showCamInHUD && isDebugging)
	{
		DrawMaterialSimple(M_TextureCam, (SizeX - TextureCam_ScreenW) / 2.0f, (SizeY - TextureCam_ScreenH) / 2.0f, TextureCam_ScreenW, TextureCam_ScreenH);
	}
}

void AROXHUD::DrawStateString(int32 SizeX, int32 SizeY)
{
	if (isDebugging)
	{
		if (isProfiling)
		{
			DrawText("PROFILING IN PROGRESS", FLinearColor::Blue, SizeX / ScreenPosFactor_X, SizeY / ScreenPosFactor_Y + 15.0f);
		}

		if (stateStringRemainingTime > 0)
		{
			DrawText(stateString, FLinearColor::Black, SizeX / ScreenPosFactor_X, SizeY / ScreenPosFactor_Y + 30.0f);
			stateStringRemainingTime -= GetWorld()->DeltaTimeSeconds;
		}
	}
}

void AROXHUD::DrawErrorString(int32 SizeX, int32 SizeY)
{
	if (errorStringRemainingTime > 0)
	{
		DrawText(errorString, FLinearColor::Red, SizeX / ScreenPosFactor_X, SizeY / ScreenPosFactor_Y + 45.0f);
		errorStringRemainingTime -= GetWorld()->DeltaTimeSeconds;
	}
}

bool AROXHUD::HudResetVR_Implementation()
{
	ShowStateString("Reset VR", 2.0f);
	return true;
}

bool AROXHUD::ToggleDebugging_Implementation()
{
	isDebugging = !isDebugging;
	if (!isDebugging && isProfiling)
	{
		GetOwningPlayerController()->ConsoleCommand("stat stopfile");
		isProfiling = false;
	}
	return isDebugging;
}

bool AROXHUD::ToggleCamTexture_Implementation()
{
	showCamInHUD = !showCamInHUD;
	ShowStateString("Cam Texture", 2.0f);
	
	return showCamInHUD;
}

bool AROXHUD::ToggleRecording_Implementation()
{
	isRecording = !isRecording;
	return isRecording;
}

bool AROXHUD::ToggleProfiling_Implementation()
{
	isProfiling = !isProfiling;
	GetOwningPlayerController()->ConsoleCommand(isProfiling ? "stat startfile" : "stat stopfile");
	ShowStateString(isProfiling ? "Profiling started...." : "Profiling stopped....", 2.0f);
	return isProfiling;
}

bool AROXHUD::HudShowState_Implementation(const FString &state, const float &timeSeconds)
{
	ShowStateString(state, timeSeconds);
	return true;
}

bool AROXHUD::HudShowError_Implementation(const FString &error)
{
	ShowErrorString(error, 30.f);
	return true;
}

void AROXHUD::ShowStateString(FString newStateString, float timeSeconds)
{
	stateString = newStateString;
	stateStringRemainingTime = timeSeconds;
}

void AROXHUD::ShowErrorString(FString newErrorString, float timeSeconds)
{
	errorString = newErrorString;
	errorStringRemainingTime = timeSeconds;
}
