// Fill out your copyright notice in the Description page of Project Settings.

#include "HUDInteractionInterface.h"


// Add default functionality here for any IHUDInteractionInterface functions that are not pure virtual.

bool IHUDInteractionInterface::ToggleRecording_Implementation() {
	return true;
}

bool IHUDInteractionInterface::ToggleDebugging_Implementation() {
	return true;
}

bool IHUDInteractionInterface::ToggleProfiling_Implementation()
{
	return true;
}

bool IHUDInteractionInterface::ToggleCamTexture_Implementation() {
	return true;
}

bool IHUDInteractionInterface::HudResetVR_Implementation() {
	return true;
}

bool IHUDInteractionInterface::HudShowState_Implementation(const FString &state, const float &timeSeconds)
{
	return false;
}

bool IHUDInteractionInterface::HudShowError_Implementation(const FString &error)
{
	return false;
}
