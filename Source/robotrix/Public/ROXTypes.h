// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraActor.h"
#include "ROXTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUnrealROX, Log, All);

USTRUCT()
struct FROXActorState
{
	GENERATED_USTRUCT_BODY()

	FVector Position;
	FRotator Rotation;

	FROXActorState()
	{}
};

USTRUCT()
struct FROXActorStateExtended
{
	GENERATED_USTRUCT_BODY()

	FVector Position;
	FRotator Rotation;
	FVector BoundingBox_Min;
	FVector BoundingBox_Max;

	FROXActorStateExtended()
	{}
};

USTRUCT()
struct FROXSkeletonState
{
	GENERATED_USTRUCT_BODY()

	FVector Position;
	FRotator Rotation;
	TMap<FString, FROXActorState> Bones;

	FROXSkeletonState()
	{}
};

USTRUCT()
struct FROXFrame
{
	GENERATED_USTRUCT_BODY()

	int n_frame;
	int n_frame_generated;
	float time_stamp;
	TMap<FString, FROXActorState> Cameras;
	TMap<FString, FROXActorStateExtended> Objects;
	TMap<FString, FROXSkeletonState> Skeletons;

	FROXFrame()
	{}
};

USTRUCT()
struct FROXCameraConfig
{
	GENERATED_USTRUCT_BODY()

	FString CameraName;
	float StereoBaseline;
	float FieldOfView;

	FROXCameraConfig()
	{}
};

USTRUCT()
struct FROXPawnInfo
{
	GENERATED_USTRUCT_BODY()

	FString PawnName;
	int NumBones;

	FROXPawnInfo()
	{}
};

USTRUCT()
struct FROXMeshComponentMaterials
{
	GENERATED_USTRUCT_BODY()

	UMeshComponent* MeshComponent;
	TArray<UMaterialInterface*> DefaultMaterials;
	FLinearColor MaskMaterialLinearColor;
	FColor MaskMaterialColor;
	UMaterialInstanceDynamic* MaskMaterial;

	FROXMeshComponentMaterials()
	{}
};

USTRUCT()
struct FROXSceneObject
{
	GENERATED_USTRUCT_BODY()

	FName instance_name;
	FColor instance_color;
	FName instance_class;

	FROXSceneObject()
	{}
};