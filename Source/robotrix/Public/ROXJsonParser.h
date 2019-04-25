// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Json.h"
#include "ROXTypes.h"

/**
 * 
 */
class ROBOTRIX_API ROXJsonParser
{
public:
	ROXJsonParser();
	~ROXJsonParser();

	bool LoadFile(FString filename);
	FROXFrame GetFrameData(uint64 nFrame);
	static FString IntToStringDigits(int i, int nDigits);
	static void SceneTxtToJson(FString path, FString txt_filename, FString json_filename);

	static TSharedPtr<FJsonObject> FColorToJson(FColor color);
	static FColor JsonToFColor(TSharedPtr<FJsonObject> JsonObject);
	static TMap<FName, FROXSceneObject> LoadSceneObjects(FString filename);
	static bool WriteSceneObjects(TMap<AActor*, TArray<FROXMeshComponentMaterials>> MeshMaterials, FString filename);

	FORCEINLINE uint64 GetNumFrames() const
	{
		return NumFrames;
	}

	FORCEINLINE FString GetSequenceName() const
	{
		return SequenceName;
	}

	FORCEINLINE float GetTotalTime() const
	{
		return TotalTime;
	}

	FORCEINLINE float GetMeanFramerate() const
	{
		return MeanFramerate;
	}

	FORCEINLINE TArray<FString> GetCameraNames() const
	{
		return CameraNames;
	}

	FORCEINLINE TArray<FROXCameraConfig> GetCameraConfigs() const
	{
		return CameraConfigs;
	}

	FORCEINLINE TArray<FString> GetPawnNames() const
	{
		return PawnNames;
	}

	FORCEINLINE TArray<FROXPawnInfo> GetPawnsInfo() const
	{
		return PawnsInfo;
	}

protected:
	uint64 NumFrames;
	FString SequenceName;
	float TotalTime;
	float MeanFramerate;

	TArray<FString> PawnNames;
	TArray<FString> CameraNames;
	TArray<FROXCameraConfig> CameraConfigs;
	TArray<FROXPawnInfo> PawnsInfo;
	TArray<TSharedPtr<FJsonValue>> CamerasJsonArray;
	TArray<TSharedPtr<FJsonValue>> FramesJsonArray;
	TArray<TSharedPtr<FJsonValue>> PawnsJsonArray;
};
