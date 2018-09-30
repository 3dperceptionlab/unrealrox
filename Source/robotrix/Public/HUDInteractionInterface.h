// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HUDInteractionInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UHUDInteractionInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ROBOTRIX_API IHUDInteractionInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Gameplay)
	bool ToggleRecording();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Gameplay)
	bool ToggleDebugging();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Gameplay)
	bool ToggleProfiling();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Gameplay)
	bool ToggleCamTexture();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Gameplay)
	bool HudResetVR();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Gameplay)
	bool HudShowState(const FString &state, const float &timeSeconds);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Gameplay)
	bool HudShowError(const FString &error);

};
