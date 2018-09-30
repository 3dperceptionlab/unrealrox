// Fill out your copyright notice in the Description page of Project Settings.

#include "ROXBasePawn.h"
#include "HUDInteractionInterface.h"
#include "IXRTrackingSystem.h"
#include "ROXHUD.h"
#include "Runtime/Core/Public/Misc/MessageDialog.h"
#include "ROXPlayerController.h"


// Sets default values
AROXBasePawn::AROXBasePawn()
	: bRecordMode(true)
	, bDebugMode(false)
	, SpeedModifier(250.f)
	, isHMDEnabled(false)
	, bScaleHeadSmall(false)
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	MeshComponent = CreateOptionalDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	MeshComponent->bEnableUpdateRateOptimizations = false;
	MeshComponent->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::AlwaysTickPoseAndRefreshBones;

	VRTripod = CreateDefaultSubobject<USceneComponent>(TEXT("VRTripod"));
	VRTripod->SetupAttachment(RootComponent);
	//VRTripod->SetRelativeLocation(RelativeCameraPosition);

	VRCamera = CreateDefaultSubobject<USceneComponent>(TEXT("VRCamera"));
	VRCamera->SetupAttachment(VRTripod);

	PawnCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PawnCamera"));
	PawnCamera->SetupAttachment(VRCamera);
	//PawnCamera->SetRelativeLocation(RelativeCameraPosition);

	VROrigin = CreateDefaultSubobject<USceneComponent>(TEXT("VROrigin"));
	VROrigin->SetupAttachment(VRTripod);

	MotionController_R = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionController_R"));
	MotionController_R->SetupAttachment(VROrigin);
	MotionController_R->Hand = EControllerHand::Right;

	MotionController_L = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionController_L"));
	MotionController_L->SetupAttachment(VROrigin);
	MotionController_L->Hand = EControllerHand::Left;

	AutoPossessPlayer = EAutoReceiveInput::Player0;
	AIControllerClass = AROXPlayerController::StaticClass();

	SetupHandsCapsuleColliders();
}

// Called when the game starts or when spawned
void AROXBasePawn::BeginPlay()
{
	Super::BeginPlay();
	if (GEngine->XRSystem.IsValid())
	{
		if (GEngine->XRSystem->IsHeadTrackingAllowed())
		{
			isHMDEnabled = true;
		}
	}
	
	if (!isHMDEnabled)
	{
		PawnCamera->SetRelativeLocation(RelativeCameraPosition);
	}

	// Grasping Maps initialization
	for (const EHandFinger Finger : EHandFingerAll)
	{
		R_FingerGrips.Emplace(Finger, 0.0f);
		R_FingerOverlapping.Emplace(Finger, false);
		R_FingerBlocked.Emplace(Finger, false);
		L_FingerGrips.Emplace(Finger, 0.0f);
		L_FingerOverlapping.Emplace(Finger, false);
		L_FingerBlocked.Emplace(Finger, false);
	}

	// Init NameTransformMap
	for (uint8 i = 0; i < MeshComponent->GetNumBones(); ++i)
	{
		REC_NameTransformMap.Emplace(MeshComponent->GetBoneName(i), FTransform());
	}

	UpdateBoneCamsArray();
	SetupHandsCapsuleCollidersAttachment();

	CachedPC = Cast<AROXPlayerController>(GetController());
}

void AROXBasePawn::InitFromTracker(bool InRecordMode, bool InDebugMode, class AROXTracker* InTracker)
{
	bRecordMode = InRecordMode;
	bDebugMode = InDebugMode;
	Tracker = InTracker;

	if (!bRecordMode)
	{
		// Disable input through PlayerController
		AROXPlayerController* PC = Cast<AROXPlayerController>(GetController());
		PC->DisableInput(PC);
		// Disable HUD
		PC->GetHUD()->ShowHUD();
		HideHandsCapsuleColliders();
	}
}

void AROXBasePawn::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateBoneCamsArray();
}

void AROXBasePawn::UpdateBoneCamsArray()
{
	// Preventing duplicated cameras
	for (int i = 0; i < BoneCams.Num(); ++i)
	{
		for (int j = i+1; j < BoneCams.Num(); ++j)
		{
			if (BoneCams[i].CameraActor &&
				BoneCams[i].CameraActor == BoneCams[j].CameraActor)
			{
				BoneCams[j].CameraActor = nullptr;
				FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("ERROR: Cannot attach the same camera twice"));
			}
		}
	}

	// Detaching deselected cameras
	bool found(false);
	for (int i = 0; i < CachedBoneCams_PT.Num(); ++i)
	{
		for (int j = 0; j < BoneCams.Num(); ++j)
		{
			if ((CachedBoneCams_PT[i].CameraActor) && 
				CachedBoneCams_PT[i].CameraActor == BoneCams[j].CameraActor)
			{
				found = true;
			}
		}
		if (!found && CachedBoneCams_PT[i].CameraActor)
		{
			CachedBoneCams_PT[i].CameraActor->DetachFromActor(FDetachmentTransformRules(EDetachmentRule::KeepRelative, true));
			CachedBoneCams_PT.RemoveAt(i);
		}
		found = false;
	}
	
	// Attaching selected cameras and keep world
	for (int i = 0; i < BoneCams.Num(); ++i)
	{
		if (BoneCams[i].CameraActor)
		{
			BoneCams[i].CameraActor->AttachToComponent(MeshComponent, FAttachmentTransformRules(EAttachmentRule::KeepWorld, true), BoneCams[i].SocketName);
			for (int j = 0; j < CachedBoneCams_PT.Num(); ++j)
			{
				if (BoneCams[i].CameraActor == CachedBoneCams_PT[j].CameraActor
					&& BoneCams[i].SocketName != CachedBoneCams_PT[j].SocketName)
				{
					BoneCams[i].CameraActor->SetActorRelativeTransform(FTransform());
				}
			}
		}
	}

	CachedBoneCams_PT = BoneCams;
	RegisterAllComponents();
}

void AROXBasePawn::EmplaceBoneTransformMap(FName name, FTransform transform)
{
	REC_NameTransformMap.Emplace(name, transform);
}

FTransform AROXBasePawn::GetPawnCameraTransform()
{
	//PawnCamera->GetComponentTransform();
	return FTransform(PawnCamera->GetComponentRotation(), PawnCamera->GetComponentLocation());
}

void AROXBasePawn::MoveCameraRelative(FVector position_offset)
{
	VRCamera->SetRelativeLocation(VRCamera->GetRelativeTransform().GetLocation() + position_offset);
}

void AROXBasePawn::MoveVRCameraControllersRelative(FVector position_offset)
{
	VRTripod->SetRelativeLocation(VRTripod->GetRelativeTransform().GetLocation() + position_offset);
}

// Called every frame
void AROXBasePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bRecordMode) //Record Mode
	{
		if (PawnCameraSub)
		{
			PawnCameraSub->SetActorTransform(GetPawnCameraTransform());
		}
	}
}

void AROXBasePawn::CameraPitchRotation(float Value)
{
	PawnCamera->SetRelativeRotation(FRotator(	PawnCamera->GetRelativeTransform().Rotator().Pitch + Value * 1.0f,
												PawnCamera->GetRelativeTransform().Rotator().Yaw,
												PawnCamera->GetRelativeTransform().Rotator().Roll));
}

void AROXBasePawn::ChangeViewTarget(AActor* ViewTarget)
{
	if (CachedPC)
	{
		CachedPC->SetViewTarget(ViewTarget);
	}
}

bool AROXBasePawn::CheckFirstPersonCamera(ACameraActor* inputCam)
{
	bool isFirstPersonCamera = false;
	if (PawnCameraSub && inputCam->GetName() == PawnCameraSub->GetName())
	{
		bScaleHeadSmall = true;
		isFirstPersonCamera = true;
	}
	else
	{
		bScaleHeadSmall = false;
	}
	return isFirstPersonCamera;
}

void AROXBasePawn::GraspRightHand(float Value)
{
	Grasp(Value, R_FingerGrips, R_FingerOverlapping, R_FingerBlocked, FName("hand_r"), R_OverlappedActor, R_isActorAttached, R_DisableGrab, R_AttachedActor, L_isActorAttached, L_DisableGrab, L_AttachedActor);
	R_CurrentGrip = Value;
}

void AROXBasePawn::GraspLeftHand(float Value)
{
	Grasp(Value, L_FingerGrips, L_FingerOverlapping, L_FingerBlocked, FName("hand_l"), L_OverlappedActor, L_isActorAttached, L_DisableGrab, L_AttachedActor, R_isActorAttached, R_DisableGrab, R_AttachedActor);
	L_CurrentGrip = Value;
}

void AROXBasePawn::Grasp(float CurrentGrip, TMap<EHandFinger, float> &FingerGrips, TMap<EHandFinger, bool> &FingerOverlapping, TMap<EHandFinger, bool> &FingerBlocked, FName BoneName, AActor* &OverlappedActor, bool &isActorAttached, bool &DisableGrab, AActor* &AttachedActor, bool &isActorAttachedOtherHand, bool &DisableGrabOtherHand, AActor* &AttachedActorOtherHand)
{
	// Update finger positions
	for (auto Entry = FingerBlocked.CreateIterator(); Entry; ++Entry)
	{
		const EHandFinger& Finger = Entry.Key();
		const bool& bFingerBlocked = Entry.Value();

		float fFingerGrip = *FingerGrips.Find(Finger);
		bool bFingerOverlapping = *FingerOverlapping.Find(Finger);
		bool bSmoothGrasp = false;

		if (bFingerBlocked)
		{
			if (CurrentGrip < fFingerGrip)
			{
				FingerBlocked.Emplace(Finger, false);
				bSmoothGrasp = true;
			}
		}
		else
		{
			bSmoothGrasp = true;
		}

		if (bSmoothGrasp)
		{
			SmoothGrasp(FingerGrips, Finger, CurrentGrip, 0.6f);
		}
	}

	// Detect grasp and attach object to hand
	bool isThumbOverlapping = *FingerOverlapping.Find(EHandFinger::HF_Thumb_3);
	bool isIndexOverlapping = *FingerOverlapping.Find(EHandFinger::HF_Index_3);
	bool isMiddleOverlapping = *FingerOverlapping.Find(EHandFinger::HF_Middle_3);

	if (isThumbOverlapping && (isIndexOverlapping || isMiddleOverlapping) && CurrentGrip >= GripDeadZone)
	{
		if (!isActorAttached && !DisableGrab)
		{
			AttachedActor = OverlappedActor;
			isActorAttached = true;

			if (AttachedActor == AttachedActorOtherHand && isActorAttachedOtherHand)
			{
				DetachObject(AttachedActorOtherHand);
				isActorAttachedOtherHand = false;
				DisableGrabOtherHand = true;
			}

			AttachObject(OverlappedActor, BoneName);
		}
	}
	else
	{
		if (isActorAttached)
		{
			DetachObject(AttachedActor);
			isActorAttached = false;
		}
		DisableGrab = false;
	}
}

void AROXBasePawn::SmoothGrasp(TMap<EHandFinger, float> &FingerGrips, EHandFinger Finger, float InputGrip, float MaxStepPerSecond)
{
	float FingerGrip = *FingerGrips.Find(Finger);
	float GripDiff = InputGrip - FingerGrip;

	if (FMath::Abs(GripDiff) > FApp::GetDeltaTime() * MaxStepPerSecond)
	{
		if (GripDiff > 0.0f)
		{
			FingerGrips.Emplace(Finger, FingerGrip + FApp::GetDeltaTime() * MaxStepPerSecond);
		}
		else
		{
			FingerGrips.Emplace(Finger, FingerGrip - FApp::GetDeltaTime() * MaxStepPerSecond);
		}
	}
	else
	{
		FingerGrips.Emplace(Finger, InputGrip);
	}
}


void AROXBasePawn::AttachObject(AActor *ObjectToAttach, FName BoneName)
{
	check(ObjectToAttach);
	FRotator RotationPrev = ObjectToAttach->GetRootComponent()->GetComponentRotation();
	FVector LocationPrev = ObjectToAttach->GetRootComponent()->GetComponentLocation();

	ObjectToAttach->GetRootComponent()->AttachToComponent
	(
		(USceneComponent*)MeshComponent,
		FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, true),
		BoneName
	);

	ObjectToAttach->GetRootComponent()->SetWorldLocationAndRotation(LocationPrev, RotationPrev);
}


void AROXBasePawn::DetachObject(AActor *ObjectToDetach)
{
	check(ObjectToDetach);
	ObjectToDetach->GetRootComponent()->DetachFromComponent
	(
		FDetachmentTransformRules(EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, true)
	);
	UPrimitiveComponent *PrimComp = Cast<UPrimitiveComponent>(ObjectToDetach->GetRootComponent());
	check(PrimComp);
	PrimComp->SetSimulatePhysics(true);
	PrimComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

bool AROXBasePawn::SetOverlap(TMap<EHandFinger, bool> &FingerOverlapping, TMap<EHandFinger, bool> &FingerBlocked, EHandFinger Finger, AActor* &OverlappedActor, AActor* &AlreadyOverlappedActor, float CurrentGrip)
{
	FingerOverlapping.Emplace(Finger, true);
	AlreadyOverlappedActor = OverlappedActor;

	if (CurrentGrip >= GripDeadZone)
	{
		FingerBlocked.Emplace(Finger, true);
		PrintHUD("Finger overlap: " + EHandFingerToString(Finger));
	}

	return CurrentGrip >= GripDeadZone;
}

bool AROXBasePawn::SetOverlapEnd(TMap<EHandFinger, bool> &FingerOverlapping, EHandFinger Finger)
{
	FingerOverlapping.Emplace(Finger, false);	
	PrintHUD("Finger end overlap: " + EHandFingerToString(Finger));
	return true;
}

void AROXBasePawn::PrintHUD(FString str)
{
	AROXPlayerController *PC = Cast<AROXPlayerController>(GetController());
	if (PC->GetHUD() && PC->GetHUD()->Implements<UHUDInteractionInterface>())
	{
		IHUDInteractionInterface::Execute_HudShowState(PC->GetHUD(), str, 1.0f);
	}
}

FString AROXBasePawn::EHandFingerToString(EHandFinger Finger)
{
	FString finger_str;
	switch (Finger)
	{
	case EHandFinger::HF_Default: finger_str = "Default";
		break;
	case EHandFinger::HF_Thumb_3: finger_str = "Thumb";
		break;
	case EHandFinger::HF_Index_3: finger_str = "Index";
		break;
	case EHandFinger::HF_Middle_3: finger_str = "Middle";
		break;
	case EHandFinger::HF_Ring_3: finger_str = "Ring";
		break;
	case EHandFinger::HF_Pinky_3: finger_str = "Pinky";
		break;
	default: finger_str = "Default";
		break;
	}
	return finger_str;
}

void AROXBasePawn::HideHandsCapsuleColliders()
{
	Thumb_3R->SetHiddenInGame(true);
	Index_3R->SetHiddenInGame(true);
	Middle_3R->SetHiddenInGame(true);
	Ring_3R->SetHiddenInGame(true);
	Pinky_3R->SetHiddenInGame(true);
	Thumb_3L->SetHiddenInGame(true);
	Index_3L->SetHiddenInGame(true);
	Middle_3L->SetHiddenInGame(true);
	Ring_3L->SetHiddenInGame(true);
	Pinky_3L->SetHiddenInGame(true);
	Thumb_2R->SetHiddenInGame(true);
	Index_2R->SetHiddenInGame(true);
	Middle_2R->SetHiddenInGame(true);
	Ring_2R->SetHiddenInGame(true);
	Pinky_2R->SetHiddenInGame(true);
	Thumb_2L->SetHiddenInGame(true);
	Index_2L->SetHiddenInGame(true);
	Middle_2L->SetHiddenInGame(true);
	Ring_2L->SetHiddenInGame(true);
	Pinky_2L->SetHiddenInGame(true);
}

void AROXBasePawn::SetupHandsCapsuleColliders()
{
	Thumb_3R = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Thumb_3R"));
	Thumb_3R->SetupAttachment(MeshComponent);
	Thumb_3R->SetRelativeTransform(FTransform(FRotator(0.0f, 100.0f, 92.0f), FVector(-1.8f, 1.0f, 0.1f)));
	Thumb_3R->SetCapsuleSize(0.6f, 1.0f);
	Thumb_3R->bGenerateOverlapEvents = true;
	Thumb_3R->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	Thumb_3R->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	Thumb_3R->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	Thumb_3R->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	Thumb_3R->OnComponentBeginOverlap.AddDynamic(this, &AROXBasePawn::OnThumb_3ROverlapBegin);
	Thumb_3R->OnComponentEndOverlap.AddDynamic(this, &AROXBasePawn::OnThumb_3ROverlapEnd);
	Thumb_3R->SetHiddenInGame(false);

	Index_3R = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Index_3R"));
	Index_3R->SetupAttachment(MeshComponent);
	Index_3R->SetRelativeTransform(FTransform(FRotator(0.0f, 86.0f, 91.0f), FVector(-1.3f, 0.7f, 0.2f)));
	Index_3R->SetCapsuleSize(0.43f, 1.05f);
	Index_3R->bGenerateOverlapEvents = true;
	Index_3R->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	Index_3R->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	Index_3R->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	Index_3R->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	Index_3R->OnComponentBeginOverlap.AddDynamic(this, &AROXBasePawn::OnIndex_3ROverlapBegin);
	Index_3R->OnComponentEndOverlap.AddDynamic(this, &AROXBasePawn::OnIndex_3ROverlapEnd);
	Index_3R->SetHiddenInGame(false);

	Middle_3R = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Middle_3R"));
	Middle_3R->SetupAttachment(MeshComponent);
	Middle_3R->SetRelativeTransform(FTransform(FRotator(0.0f, 86.0f, 86.0f), FVector(-1.8f, 0.5f, 0.065f)));
	Middle_3R->SetCapsuleSize(0.53f, 1.4f);
	Middle_3R->bGenerateOverlapEvents = true;
	Middle_3R->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	Middle_3R->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	Middle_3R->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	Middle_3R->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	Middle_3R->OnComponentBeginOverlap.AddDynamic(this, &AROXBasePawn::OnMiddle_3ROverlapBegin);
	Middle_3R->OnComponentEndOverlap.AddDynamic(this, &AROXBasePawn::OnMiddle_3ROverlapEnd);
	Middle_3R->SetHiddenInGame(false);

	Ring_3R = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Ring_3R"));
	Ring_3R->SetupAttachment(MeshComponent);
	Ring_3R->SetRelativeTransform(FTransform(FRotator(0.0f, 86.0f, 97.0f), FVector(-1.6f, 0.7f, 0.0f)));
	Ring_3R->SetCapsuleSize(0.53f, 1.2f);
	Ring_3R->bGenerateOverlapEvents = true;
	Ring_3R->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	Ring_3R->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	Ring_3R->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	Ring_3R->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	Ring_3R->OnComponentBeginOverlap.AddDynamic(this, &AROXBasePawn::OnRing_3ROverlapBegin);
	Ring_3R->OnComponentEndOverlap.AddDynamic(this, &AROXBasePawn::OnRing_3ROverlapEnd);
	Ring_3R->SetHiddenInGame(false);

	Pinky_3R = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Pinky_3R"));
	Pinky_3R->SetupAttachment(MeshComponent);
	Pinky_3R->SetRelativeTransform(FTransform(FRotator(0.0f, 95.0f, 96.0f), FVector(-1.2f, 0.3f, 0.0f)));
	Pinky_3R->SetCapsuleSize(0.53f, 1.2f);
	Pinky_3R->bGenerateOverlapEvents = true;
	Pinky_3R->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	Pinky_3R->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	Pinky_3R->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	Pinky_3R->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	Pinky_3R->OnComponentBeginOverlap.AddDynamic(this, &AROXBasePawn::OnPinky_3ROverlapBegin);
	Pinky_3R->OnComponentEndOverlap.AddDynamic(this, &AROXBasePawn::OnPinky_3ROverlapEnd);
	Pinky_3R->SetHiddenInGame(false);

	Thumb_3L = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Thumb_3L"));
	Thumb_3L->SetupAttachment(MeshComponent);
	Thumb_3L->SetRelativeTransform(FTransform(FRotator(0.0f, 90.0f, 90.0f), FVector(1.8f, -1.1f, 0.0f)));
	Thumb_3L->SetCapsuleSize(0.6f, 1.12f);
	Thumb_3L->bGenerateOverlapEvents = true;
	Thumb_3L->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	Thumb_3L->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	Thumb_3L->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	Thumb_3L->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	Thumb_3L->OnComponentBeginOverlap.AddDynamic(this, &AROXBasePawn::OnThumb_3LOverlapBegin);
	Thumb_3L->OnComponentEndOverlap.AddDynamic(this, &AROXBasePawn::OnThumb_3LOverlapEnd);
	Thumb_3L->SetHiddenInGame(false);

	Index_3L = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Index_3L"));
	Index_3L->SetupAttachment(MeshComponent);
	Index_3L->SetRelativeTransform(FTransform(FRotator(0.0f, 86.0f, 88.0f), FVector(1.2f, -0.8f, -0.15f)));
	Index_3L->SetCapsuleSize(0.43f, 1.05f);
	Index_3L->bGenerateOverlapEvents = true;
	Index_3L->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	Index_3L->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	Index_3L->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	Index_3L->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	Index_3L->OnComponentBeginOverlap.AddDynamic(this, &AROXBasePawn::OnIndex_3LOverlapBegin);
	Index_3L->OnComponentEndOverlap.AddDynamic(this, &AROXBasePawn::OnIndex_3LOverlapEnd);
	Index_3L->SetHiddenInGame(false);

	Middle_3L = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Middle_3L"));
	Middle_3L->SetupAttachment(MeshComponent);
	Middle_3L->SetRelativeTransform(FTransform(FRotator(0.0f, 86.0f, 86.0f), FVector(1.0f, -0.55f, 0.0f)));
	Middle_3L->SetCapsuleSize(0.53f, 1.35f);
	Middle_3L->bGenerateOverlapEvents = true;
	Middle_3L->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	Middle_3L->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	Middle_3L->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	Middle_3L->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	Middle_3L->OnComponentBeginOverlap.AddDynamic(this, &AROXBasePawn::OnMiddle_3LOverlapBegin);
	Middle_3L->OnComponentEndOverlap.AddDynamic(this, &AROXBasePawn::OnMiddle_3LOverlapEnd);
	Middle_3L->SetHiddenInGame(false);

	Ring_3L = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Ring_3L"));
	Ring_3L->SetupAttachment(MeshComponent);
	Ring_3L->SetRelativeTransform(FTransform(FRotator(0.0f, 86.5f, 94.0f), FVector(1.0f, -0.6f, 0.0f)));
	Ring_3L->SetCapsuleSize(0.53f, 1.2f);
	Ring_3L->bGenerateOverlapEvents = true;
	Ring_3L->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	Ring_3L->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	Ring_3L->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	Ring_3L->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	Ring_3L->OnComponentBeginOverlap.AddDynamic(this, &AROXBasePawn::OnRing_3LOverlapBegin);
	Ring_3L->OnComponentEndOverlap.AddDynamic(this, &AROXBasePawn::OnRing_3LOverlapEnd);
	Ring_3L->SetHiddenInGame(false);

	Pinky_3L = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Pinky_3L"));
	Pinky_3L->SetupAttachment(MeshComponent);
	Pinky_3L->SetRelativeTransform(FTransform(FRotator(0.0f, 95.0f, 92.0f), FVector(1.0f, -0.4f, 0.05f)));
	Pinky_3L->SetCapsuleSize(0.44f, 1.02f);
	Pinky_3L->bGenerateOverlapEvents = true;
	Pinky_3L->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	Pinky_3L->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	Pinky_3L->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	Pinky_3L->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	Pinky_3L->OnComponentBeginOverlap.AddDynamic(this, &AROXBasePawn::OnPinky_3LOverlapBegin);
	Pinky_3L->OnComponentEndOverlap.AddDynamic(this, &AROXBasePawn::OnPinky_3LOverlapEnd);
	Pinky_3L->SetHiddenInGame(false);

	Thumb_2R = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Thumb_2R"));
	Thumb_2R->SetupAttachment(MeshComponent);
	Thumb_2R->SetRelativeTransform(FTransform(FRotator(0.0f, 95.0f, 92.0f), FVector(-2.7f, 0.6f, -0.05f)));
	Thumb_2R->SetCapsuleSize(1.1f, 1.45f);
	Thumb_2R->bGenerateOverlapEvents = true;
	Thumb_2R->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	Thumb_2R->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	Thumb_2R->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	Thumb_2R->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	Thumb_2R->OnComponentBeginOverlap.AddDynamic(this, &AROXBasePawn::OnThumb_2ROverlapBegin);
	Thumb_2R->OnComponentEndOverlap.AddDynamic(this, &AROXBasePawn::OnThumb_2ROverlapEnd);
	Thumb_2R->SetHiddenInGame(false);

	Index_2R = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Index_2R"));
	Index_2R->SetupAttachment(MeshComponent);
	Index_2R->SetRelativeTransform(FTransform(FRotator(0.0f, 90.0f, 88.0f), FVector(-1.6f, 0.6f, 0.2f)));
	Index_2R->SetCapsuleSize(0.5f, 1.12f);
	Index_2R->bGenerateOverlapEvents = true;
	Index_2R->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	Index_2R->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	Index_2R->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	Index_2R->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	Index_2R->OnComponentBeginOverlap.AddDynamic(this, &AROXBasePawn::OnIndex_2ROverlapBegin);
	Index_2R->OnComponentEndOverlap.AddDynamic(this, &AROXBasePawn::OnIndex_2ROverlapEnd);
	Index_2R->SetHiddenInGame(false);

	Middle_2R = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Middle_2R"));
	Middle_2R->SetupAttachment(MeshComponent);
	Middle_2R->SetRelativeTransform(FTransform(FRotator(0.0f, 93.0f, 90.0f), FVector(-1.6f, 0.7f, 0.065f)));
	Middle_2R->SetCapsuleSize(0.53f, 1.4f);
	Middle_2R->bGenerateOverlapEvents = true;
	Middle_2R->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	Middle_2R->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	Middle_2R->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	Middle_2R->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	Middle_2R->OnComponentBeginOverlap.AddDynamic(this, &AROXBasePawn::OnMiddle_2ROverlapBegin);
	Middle_2R->OnComponentEndOverlap.AddDynamic(this, &AROXBasePawn::OnMiddle_2ROverlapEnd);
	Middle_2R->SetHiddenInGame(false);

	Ring_2R = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Ring_2R"));
	Ring_2R->SetupAttachment(MeshComponent);
	Ring_2R->SetRelativeTransform(FTransform(FRotator(0.0f, 90.0f, 90.0f), FVector(-1.5f, 0.6f, 0.0f)));
	Ring_2R->SetCapsuleSize(0.53f, 1.2f);
	Ring_2R->bGenerateOverlapEvents = true;
	Ring_2R->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	Ring_2R->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	Ring_2R->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	Ring_2R->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	Ring_2R->OnComponentBeginOverlap.AddDynamic(this, &AROXBasePawn::OnRing_2ROverlapBegin);
	Ring_2R->OnComponentEndOverlap.AddDynamic(this, &AROXBasePawn::OnRing_2ROverlapEnd);
	Ring_2R->SetHiddenInGame(false);

	Pinky_2R = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Pinky_2R"));
	Pinky_2R->SetupAttachment(MeshComponent);
	Pinky_2R->SetRelativeTransform(FTransform(FRotator(0.0f, 92.0f, 89.0f), FVector(-1.5f, 0.6f, 0.0f)));
	Pinky_2R->SetCapsuleSize(0.53f, 1.0f);
	Pinky_2R->bGenerateOverlapEvents = true;
	Pinky_2R->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	Pinky_2R->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	Pinky_2R->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	Pinky_2R->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	Pinky_2R->OnComponentBeginOverlap.AddDynamic(this, &AROXBasePawn::OnPinky_2ROverlapBegin);
	Pinky_2R->OnComponentEndOverlap.AddDynamic(this, &AROXBasePawn::OnPinky_2ROverlapEnd);
	Pinky_2R->SetHiddenInGame(false);

	Thumb_2L = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Thumb_2L"));
	Thumb_2L->SetupAttachment(MeshComponent);
	Thumb_2L->SetRelativeTransform(FTransform(FRotator(0.0f, 95.0f, 92.0f), FVector(2.2f, -0.8f, 0.0f)));
	Thumb_2L->SetCapsuleSize(1.1f, 1.46f);
	Thumb_2L->bGenerateOverlapEvents = true;
	Thumb_2L->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	Thumb_2L->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	Thumb_2L->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	Thumb_2L->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	Thumb_2L->OnComponentBeginOverlap.AddDynamic(this, &AROXBasePawn::OnThumb_2LOverlapBegin);
	Thumb_2L->OnComponentEndOverlap.AddDynamic(this, &AROXBasePawn::OnThumb_2LOverlapEnd);
	Thumb_2L->SetHiddenInGame(false);

	Index_2L = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Index_2L"));
	Index_2L->SetupAttachment(MeshComponent);
	Index_2L->SetRelativeTransform(FTransform(FRotator(0.0f, 90.0f, 86.0f), FVector(1.5f, -0.7f, 0.0f)));
	Index_2L->SetCapsuleSize(0.5, 1.12f);
	Index_2L->bGenerateOverlapEvents = true;
	Index_2L->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	Index_2L->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	Index_2L->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	Index_2L->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	Index_2L->OnComponentBeginOverlap.AddDynamic(this, &AROXBasePawn::OnIndex_2LOverlapBegin);
	Index_2L->OnComponentEndOverlap.AddDynamic(this, &AROXBasePawn::OnIndex_2LOverlapEnd);
	Index_2L->SetHiddenInGame(false);

	Middle_2L = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Middle_2L"));
	Middle_2L->SetupAttachment(MeshComponent);
	Middle_2L->SetRelativeTransform(FTransform(FRotator(0.0f, 97.0f, 90.0f), FVector(1.2f, -0.6f, 0.0f)));
	Middle_2L->SetCapsuleSize(0.53f, 1.4f);
	Middle_2L->bGenerateOverlapEvents = true;
	Middle_2L->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	Middle_2L->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	Middle_2L->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	Middle_2L->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	Middle_2L->OnComponentBeginOverlap.AddDynamic(this, &AROXBasePawn::OnMiddle_2LOverlapBegin);
	Middle_2L->OnComponentEndOverlap.AddDynamic(this, &AROXBasePawn::OnMiddle_2LOverlapEnd);
	Middle_2L->SetHiddenInGame(false);

	Ring_2L = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Ring_2L"));
	Ring_2L->SetupAttachment(MeshComponent);
	Ring_2L->SetRelativeTransform(FTransform(FRotator(0.0f, 94.0f, 90.0f), FVector(1.4f, -0.6f, 0.0f)));
	Ring_2L->SetCapsuleSize(0.53f, 1.2f);
	Ring_2L->bGenerateOverlapEvents = true;
	Ring_2L->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	Ring_2L->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	Ring_2L->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	Ring_2L->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	Ring_2L->OnComponentBeginOverlap.AddDynamic(this, &AROXBasePawn::OnRing_2LOverlapBegin);
	Ring_2L->OnComponentEndOverlap.AddDynamic(this, &AROXBasePawn::OnRing_2LOverlapEnd);
	Ring_2L->SetHiddenInGame(false);

	Pinky_2L = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Pinky_2L"));
	Pinky_2L->SetupAttachment(MeshComponent);
	Pinky_2L->SetRelativeTransform(FTransform(FRotator(0.0f, 92.0f, 88.0f), FVector(1.5f, -0.5f, 0.0f)));
	Pinky_2L->SetCapsuleSize(0.53f, 1.0f);
	Pinky_2L->bGenerateOverlapEvents = true;
	Pinky_2L->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	Pinky_2L->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	Pinky_2L->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	Pinky_2L->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	Pinky_2L->OnComponentBeginOverlap.AddDynamic(this, &AROXBasePawn::OnPinky_2LOverlapBegin);
	Pinky_2L->OnComponentEndOverlap.AddDynamic(this, &AROXBasePawn::OnPinky_2LOverlapEnd);
	Pinky_2L->SetHiddenInGame(false);
}

void AROXBasePawn::SetupHandsCapsuleCollidersAttachment()
{
	Thumb_3R->AttachToComponent(MeshComponent, FAttachmentTransformRules::KeepRelativeTransform, FName(TEXT("thumb_03_r")));
	Index_3R->AttachToComponent(MeshComponent, FAttachmentTransformRules::KeepRelativeTransform, FName(TEXT("index_03_r")));
	Middle_3R->AttachToComponent(MeshComponent, FAttachmentTransformRules::KeepRelativeTransform, FName(TEXT("middle_03_r")));
	Ring_3R->AttachToComponent(MeshComponent, FAttachmentTransformRules::KeepRelativeTransform, FName(TEXT("ring_03_r")));
	Pinky_3R->AttachToComponent(MeshComponent, FAttachmentTransformRules::KeepRelativeTransform, FName(TEXT("pinky_03_r")));
	Thumb_3L->AttachToComponent(MeshComponent, FAttachmentTransformRules::KeepRelativeTransform, FName(TEXT("thumb_03_l")));
	Index_3L->AttachToComponent(MeshComponent, FAttachmentTransformRules::KeepRelativeTransform, FName(TEXT("index_03_l")));
	Middle_3L->AttachToComponent(MeshComponent, FAttachmentTransformRules::KeepRelativeTransform, FName(TEXT("middle_03_l")));
	Ring_3L->AttachToComponent(MeshComponent, FAttachmentTransformRules::KeepRelativeTransform, FName(TEXT("ring_03_l")));
	Pinky_3L->AttachToComponent(MeshComponent, FAttachmentTransformRules::KeepRelativeTransform, FName(TEXT("pinky_03_l")));
	Thumb_2R->AttachToComponent(MeshComponent, FAttachmentTransformRules::KeepRelativeTransform, FName(TEXT("thumb_02_r")));
	Index_2R->AttachToComponent(MeshComponent, FAttachmentTransformRules::KeepRelativeTransform, FName(TEXT("index_02_r")));
	Middle_2R->AttachToComponent(MeshComponent, FAttachmentTransformRules::KeepRelativeTransform, FName(TEXT("middle_02_r")));
	Ring_2R->AttachToComponent(MeshComponent, FAttachmentTransformRules::KeepRelativeTransform, FName(TEXT("ring_02_r")));
	Pinky_2R->AttachToComponent(MeshComponent, FAttachmentTransformRules::KeepRelativeTransform, FName(TEXT("pinky_02_r")));
	Thumb_2L->AttachToComponent(MeshComponent, FAttachmentTransformRules::KeepRelativeTransform, FName(TEXT("thumb_02_l")));
	Index_2L->AttachToComponent(MeshComponent, FAttachmentTransformRules::KeepRelativeTransform, FName(TEXT("index_02_l")));
	Middle_2L->AttachToComponent(MeshComponent, FAttachmentTransformRules::KeepRelativeTransform, FName(TEXT("middle_02_l")));
	Ring_2L->AttachToComponent(MeshComponent, FAttachmentTransformRules::KeepRelativeTransform, FName(TEXT("ring_02_l")));
	Pinky_2L->AttachToComponent(MeshComponent, FAttachmentTransformRules::KeepRelativeTransform, FName(TEXT("pinky_02_l")));
}

void AROXBasePawn::OnThumb_3ROverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	SetOverlap(R_FingerOverlapping, R_FingerBlocked, EHandFinger::HF_Thumb_3, OtherActor, R_OverlappedActor, R_CurrentGrip);
}
void AROXBasePawn::OnThumb_3ROverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	SetOverlapEnd(R_FingerOverlapping, EHandFinger::HF_Thumb_3);
}
void AROXBasePawn::OnIndex_3ROverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	SetOverlap(R_FingerOverlapping, R_FingerBlocked, EHandFinger::HF_Index_3, OtherActor, R_OverlappedActor, R_CurrentGrip);
}
void AROXBasePawn::OnIndex_3ROverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	SetOverlapEnd(R_FingerOverlapping, EHandFinger::HF_Index_3);
}
void AROXBasePawn::OnMiddle_3ROverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	SetOverlap(R_FingerOverlapping, R_FingerBlocked, EHandFinger::HF_Middle_3, OtherActor, R_OverlappedActor, R_CurrentGrip);
}
void AROXBasePawn::OnMiddle_3ROverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	SetOverlapEnd(R_FingerOverlapping, EHandFinger::HF_Middle_3);
}
void AROXBasePawn::OnRing_3ROverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	SetOverlap(R_FingerOverlapping, R_FingerBlocked, EHandFinger::HF_Ring_3, OtherActor, R_OverlappedActor, R_CurrentGrip);
}
void AROXBasePawn::OnRing_3ROverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	SetOverlapEnd(R_FingerOverlapping, EHandFinger::HF_Ring_3);
}
void AROXBasePawn::OnPinky_3ROverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	SetOverlap(R_FingerOverlapping, R_FingerBlocked, EHandFinger::HF_Pinky_3, OtherActor, R_OverlappedActor, R_CurrentGrip);
}
void AROXBasePawn::OnPinky_3ROverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	SetOverlapEnd(R_FingerOverlapping, EHandFinger::HF_Pinky_3);
}
void AROXBasePawn::OnThumb_3LOverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	SetOverlap(L_FingerOverlapping, L_FingerBlocked, EHandFinger::HF_Thumb_3, OtherActor, L_OverlappedActor, L_CurrentGrip);
}
void AROXBasePawn::OnThumb_3LOverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	SetOverlapEnd(L_FingerOverlapping, EHandFinger::HF_Thumb_3);
}
void AROXBasePawn::OnIndex_3LOverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	SetOverlap(L_FingerOverlapping, L_FingerBlocked, EHandFinger::HF_Index_3, OtherActor, L_OverlappedActor, L_CurrentGrip);
}
void AROXBasePawn::OnIndex_3LOverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	SetOverlapEnd(L_FingerOverlapping, EHandFinger::HF_Index_3);
}
void AROXBasePawn::OnMiddle_3LOverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	SetOverlap(L_FingerOverlapping, L_FingerBlocked, EHandFinger::HF_Middle_3, OtherActor, L_OverlappedActor, L_CurrentGrip);
}
void AROXBasePawn::OnMiddle_3LOverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	SetOverlapEnd(L_FingerOverlapping, EHandFinger::HF_Middle_3);
}
void AROXBasePawn::OnRing_3LOverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	SetOverlap(L_FingerOverlapping, L_FingerBlocked, EHandFinger::HF_Ring_3, OtherActor, L_OverlappedActor, L_CurrentGrip);
}
void AROXBasePawn::OnRing_3LOverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	SetOverlapEnd(L_FingerOverlapping, EHandFinger::HF_Ring_3);
}
void AROXBasePawn::OnPinky_3LOverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	SetOverlap(L_FingerOverlapping, L_FingerBlocked, EHandFinger::HF_Pinky_3, OtherActor, L_OverlappedActor, L_CurrentGrip);
}
void AROXBasePawn::OnPinky_3LOverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	SetOverlapEnd(L_FingerOverlapping, EHandFinger::HF_Pinky_3);
}
void AROXBasePawn::OnThumb_2ROverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	SetOverlap(R_FingerOverlapping, R_FingerBlocked, EHandFinger::HF_Thumb_3, OtherActor, R_OverlappedActor, R_CurrentGrip);
}
void AROXBasePawn::OnThumb_2ROverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	SetOverlapEnd(R_FingerOverlapping, EHandFinger::HF_Thumb_3);
}
void AROXBasePawn::OnIndex_2ROverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	SetOverlap(R_FingerOverlapping, R_FingerBlocked, EHandFinger::HF_Index_3, OtherActor, R_OverlappedActor, R_CurrentGrip);
}
void AROXBasePawn::OnIndex_2ROverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	SetOverlapEnd(R_FingerOverlapping, EHandFinger::HF_Index_3);
}
void AROXBasePawn::OnMiddle_2ROverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	SetOverlap(R_FingerOverlapping, R_FingerBlocked, EHandFinger::HF_Middle_3, OtherActor, R_OverlappedActor, R_CurrentGrip);
}
void AROXBasePawn::OnMiddle_2ROverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	SetOverlapEnd(R_FingerOverlapping, EHandFinger::HF_Middle_3);
}
void AROXBasePawn::OnRing_2ROverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	SetOverlap(R_FingerOverlapping, R_FingerBlocked, EHandFinger::HF_Ring_3, OtherActor, R_OverlappedActor, R_CurrentGrip);
}
void AROXBasePawn::OnRing_2ROverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	SetOverlapEnd(R_FingerOverlapping, EHandFinger::HF_Ring_3);
}
void AROXBasePawn::OnPinky_2ROverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	SetOverlap(R_FingerOverlapping, R_FingerBlocked, EHandFinger::HF_Pinky_3, OtherActor, R_OverlappedActor, R_CurrentGrip);
}
void AROXBasePawn::OnPinky_2ROverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	SetOverlapEnd(R_FingerOverlapping, EHandFinger::HF_Pinky_3);
}
void AROXBasePawn::OnThumb_2LOverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	SetOverlap(L_FingerOverlapping, L_FingerBlocked, EHandFinger::HF_Thumb_3, OtherActor, L_OverlappedActor, L_CurrentGrip);
}
void AROXBasePawn::OnThumb_2LOverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	SetOverlapEnd(L_FingerOverlapping, EHandFinger::HF_Thumb_3);
}
void AROXBasePawn::OnIndex_2LOverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	SetOverlap(L_FingerOverlapping, L_FingerBlocked, EHandFinger::HF_Index_3, OtherActor, L_OverlappedActor, L_CurrentGrip);
}
void AROXBasePawn::OnIndex_2LOverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	SetOverlapEnd(L_FingerOverlapping, EHandFinger::HF_Index_3);
}
void AROXBasePawn::OnMiddle_2LOverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	SetOverlap(L_FingerOverlapping, L_FingerBlocked, EHandFinger::HF_Middle_3, OtherActor, L_OverlappedActor, L_CurrentGrip);
}
void AROXBasePawn::OnMiddle_2LOverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	SetOverlapEnd(L_FingerOverlapping, EHandFinger::HF_Middle_3);
}
void AROXBasePawn::OnRing_2LOverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	SetOverlap(L_FingerOverlapping, L_FingerBlocked, EHandFinger::HF_Ring_3, OtherActor, L_OverlappedActor, L_CurrentGrip);
}
void AROXBasePawn::OnRing_2LOverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	SetOverlapEnd(L_FingerOverlapping, EHandFinger::HF_Ring_3);
}
void AROXBasePawn::OnPinky_2LOverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	SetOverlap(L_FingerOverlapping, L_FingerBlocked, EHandFinger::HF_Pinky_3, OtherActor, L_OverlappedActor, L_CurrentGrip);
}
void AROXBasePawn::OnPinky_2LOverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	SetOverlapEnd(L_FingerOverlapping, EHandFinger::HF_Pinky_3);
}