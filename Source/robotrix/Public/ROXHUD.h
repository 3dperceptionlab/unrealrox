// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "HUDInteractionInterface.h"
#include "ROXHUD.generated.h"

/**
 * This HUD implements an interface in order to be completely decoupled from other elements of this project,
 * hence why state booleans are replicated in the class to avoid any kind of casting and cyclical dependencies.
 */
UCLASS()
class ROBOTRIX_API AROXHUD : public AHUD, public IHUDInteractionInterface
{
	GENERATED_BODY()
	
private:
	bool showCamInHUD;
	bool isRecording;
	bool isProfiling;
	bool isDebugging;
	FString stateString;
	float stateStringRemainingTime;
	FString errorString;
	float errorStringRemainingTime;

protected:
	UPROPERTY(BlueprintReadWrite)
	float ScreenPosFactor_X;
	
	UPROPERTY(BlueprintReadWrite)
	float ScreenPosFactor_Y;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UMaterialInterface* M_TextureCam;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float TextureCam_ScreenW = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float TextureCam_ScreenH = 600.0f;


	AROXHUD();

	virtual void DrawHUD() override;

	void BeginPlay() override;

	void DrawCameraTexture(int32 SizeX, int32 SizeY);
	void DrawStateString(int32 SizeX, int32 SizeY);
	void DrawErrorString(int32 SizeX, int32 SizeY);

	FORCEINLINE bool GetIsDebugging() const
	{
		return isDebugging;
	}
	
	FORCEINLINE bool GetShowCamInHUD() const
	{
		return showCamInHUD;
	}

	FORCEINLINE bool GetIsRecording() const
	{
		return isRecording;
	}


	FORCEINLINE bool GetIsProfiling() const 
	{ 
		return isProfiling; 
	}

	void ShowStateString(FString newStateString, float timeSeconds);

	void ShowErrorString(FString newErrorString, float timeSeconds);

	FORCEINLINE FString GetStateString() const
	{ 
		return stateString; 
	}

	/*******************************************************************/
	/***********  IHUDInteractionInterface Implementation **************/
	/*******************************************************************/

	virtual bool ToggleRecording_Implementation() override;
	
	virtual bool ToggleProfiling_Implementation() override;

	virtual bool ToggleCamTexture_Implementation() override;

	virtual bool HudResetVR_Implementation() override;

	virtual bool ToggleDebugging_Implementation() override;

	virtual bool HudShowState_Implementation(const FString &state, const float &timeSeconds) override;

	virtual bool HudShowError_Implementation(const FString &error) override;

};
