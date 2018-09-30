// Fill out your copyright notice in the Description page of Project Settings.

#include "ROXCameraActor.h"
#include "Camera/CameraComponent.h"
#include "ConstructorHelpers.h"

AROXCameraActor::AROXCameraActor()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh>MeshAsset(TEXT("StaticMesh'/Game/Common/ROXCamera/SM_ROXCamera.SM_ROXCamera'"));
	UStaticMesh* CamAsset = MeshAsset.Object;
	GetCameraComponent()->SetCameraMesh(CamAsset);
}

void AROXCameraActor::BeginPlay()
{
	GetCameraComponent()->Deactivate();
	//GetCameraComponent()->DestroyComponent()
	GetCameraComponent()->SetComponentTickEnabled(false);
	SetActorTickEnabled(false);
	Super::BeginPlay();	
}