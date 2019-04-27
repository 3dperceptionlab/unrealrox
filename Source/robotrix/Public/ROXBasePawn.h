// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/StaticMeshActor.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimBlueprint.h"
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MotionControllerComponent.h"
#include "Haptics/HapticFeedbackEffect_Base.h"
#include "ROXTypes.h"
#include "ROXBasePawn.generated.h"

USTRUCT()
struct FBoneCam
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	ACameraActor* CameraActor;

	UPROPERTY(EditAnywhere)
	FName SocketName;
};

UENUM(BlueprintType)
enum class EHandFinger : uint8
{
	HF_Default	UMETA(DisplayName = "Default"),
	HF_Thumb_3	UMETA(DisplayName = "Thumb_3"),
	HF_Index_3	UMETA(DisplayName = "Index_3"),
	HF_Middle_3	UMETA(DisplayName = "Middle_3"),
	HF_Ring_3	UMETA(DisplayName = "Ring_3"),
	HF_Pinky_3	UMETA(DisplayName = "Pinky_3")
};

static const EHandFinger EHandFingerAll[] = {
	  EHandFinger::HF_Default
	, EHandFinger::HF_Thumb_3
	, EHandFinger::HF_Index_3
	, EHandFinger::HF_Middle_3
	, EHandFinger::HF_Ring_3
	, EHandFinger::HF_Pinky_3
};

UCLASS()
class ROBOTRIX_API AROXBasePawn : public APawn
{
	GENERATED_BODY()

private:
	/** The main skeletal mesh associated with this Character (optional sub-object). */
	UPROPERTY(Category = Pawn, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* MeshComponent;

	UHapticFeedbackEffect_Base* GraspHapticFeedback;

protected:

	UPROPERTY(EditDefaultsOnly)
	FVector RelativeCameraPosition = FVector(-150.0f, 0.0f, 10.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UCameraComponent* PawnCamera;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UMotionControllerComponent* MotionController_R;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UMotionControllerComponent* MotionController_L;
	
	UPROPERTY(EditDefaultsOnly)
	USceneComponent* VRCamera;

	UPROPERTY(EditDefaultsOnly)
	USceneComponent* VROrigin;

	UPROPERTY(EditDefaultsOnly)
	USceneComponent* VRTripod;

	UPROPERTY(EditAnywhere, Category = Movement)
	float SpeedModifier;	

	/* Tracker actor, needed to record and rebuild sequences. Will be set by the Tracker instance in the scene. */
	UPROPERTY()
	class AROXTracker* Tracker;

	UPROPERTY()
	class AROXPlayerController* CachedPC;

	/* Array of cameras attached to a bone. Designed to be used in editor.
	 * Attachment and detachment are automatically managed in OnConstruction. */
	UPROPERTY(EditAnywhere, Category = Tracker)
	TArray<FBoneCam> BoneCams;
	/* Camera set here will be placed every frame at the same position as first person view. Used to record FP view. */
	UPROPERTY(EditAnywhere, Category = Tracker)
	ACameraActor* PawnCameraSub;
	UPROPERTY()
	TArray<FBoneCam> CachedBoneCams_PT;

	/* Map to store the transformations that must be applied to each bone.
	 * Used in Rebuild mode to receive the joint transformations from the Tracker
	 * and send them to the AnimGraph. */
	UPROPERTY(BlueprintReadWrite)
	TMap<FName, FTransform> REC_NameTransformMap;

	// Some cached properties that are not meant to change in runtime
	UPROPERTY(BlueprintReadOnly)
	bool bRecordMode;
	UPROPERTY(BlueprintReadOnly)
	bool bDebugMode;

	UPROPERTY(BlueprintReadOnly)
	bool isHMDEnabled;

	UPROPERTY(BlueprintReadOnly)
	bool bScaleHeadSmall;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform & Transform) override;
	virtual void UpdateBoneCamsArray();

	void HideHandsCapsuleColliders();
	void SetupHandsCapsuleColliders();
	void SetupHandsCapsuleCollidersAttachment();

public:
	// Sets default values for this pawn's properties
	AROXBasePawn();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void InitFromTracker(bool InRecordMode, bool InDebugMode, AROXTracker* InTracker);

	void CameraPitchRotation(float Value);

	void ChangeViewTarget(AActor* ViewTarget);

	bool CheckFirstPersonCamera(ACameraActor* inputCam);

	void MoveCameraRelative(FVector position_offset);

	void MoveVRCameraControllersRelative(FVector position_offset);

	void EmplaceBoneTransformMap(FName name, FTransform transform);

	FTransform GetPawnCameraTransform();

	FORCEINLINE bool isRecordMode()
	{
		return bRecordMode;
	}

	FORCEINLINE class AROXTracker* GetTracker()
	{
		return Tracker;
	}

	FORCEINLINE float GetSpeedModifier()
	{
		return SpeedModifier;
	}

	FORCEINLINE TMap<FName, FTransform> GetNameTransformMap()
	{
		return  REC_NameTransformMap;
	}

	/** Returns Mesh subobject **/
	FORCEINLINE USkeletalMeshComponent* GetMeshComponent() const
	{
		return MeshComponent;
	}

	//     ____                     _             
	//    / ___|_ __ __ _ ___ _ __ (_)_ __   __ _ 
	//   | |  _| '__/ _` / __| '_ \| | '_ \ / _` |
	//   | |_| | | | (_| \__ \ |_) | | | | | (_| |
	//    \____|_|  \__,_|___/ .__/|_|_| |_|\__, |
	//                       |_|            |___/ 

	//////////////////////////////////////////////
	///////////////// Variables //////////////////
	//////////////////////////////////////////////
	protected:
		UPROPERTY(BlueprintReadOnly)
		TMap<EHandFinger, float> R_FingerGrips;
		UPROPERTY(BlueprintReadOnly)
		TMap<EHandFinger, float> L_FingerGrips;

		UPROPERTY(BlueprintReadOnly)
		TMap<EHandFinger, bool> R_FingerOverlapping;
		UPROPERTY(BlueprintReadOnly)
		TMap<EHandFinger, bool> L_FingerOverlapping;

		UPROPERTY(BlueprintReadOnly)
		TMap<EHandFinger, bool> R_FingerBlocked;
		UPROPERTY(BlueprintReadOnly)
		TMap<EHandFinger, bool> L_FingerBlocked;
		
		bool L_isActorAttached = false;
		bool R_isActorAttached = false;
		bool L_DisableGrab = false;
		bool R_DisableGrab = false;
		float L_CurrentGrip = 0.0f;
		float R_CurrentGrip = 0.0f;

		AActor* L_OverlappedActor;
		AActor* R_OverlappedActor;
		AActor* L_AttachedActor;
		AActor* R_AttachedActor;

		const float GripDeadZone = 0.15f;

		TMap<AActor*, EROXMeshState> InteractionData;

	//////////////////////////////////////////////
	///////////////// Functions //////////////////
	//////////////////////////////////////////////
	public:
		/** Right hand trigger input handler **/
		void GraspRightHand(float Value);
		/** Left hand trigger input handler**/
		void GraspLeftHand(float Value);

		TMap<AActor*, EROXMeshState> GetInteractionData();
		void ResetInteractionData();

	protected:
		/** Main grasping flow **/
		bool Grasp(float InValue, TMap<EHandFinger, float> &FingerGrips, TMap<EHandFinger, bool> &FingerOverlapping, TMap<EHandFinger, bool> &FingerBlocked, FName BoneName, AActor* &AttachedActor, bool &isActorAttached, bool &DisableGrab, AActor* &OverlappedActor, bool &isActorAttachedOtherHand, bool &DisableGrabOtherHand, AActor* &AttachedActorOtherHand);

		void SmoothGrasp(TMap<EHandFinger, float> &FingerGrips, EHandFinger Finger, float InputGrip, float MaxStepPerSecond);

		void AttachObject(AActor *ObjectToAttach, FName BoneName);

		void DetachObject(AActor *ObjectToDetach);

		bool SetOverlap(TMap<EHandFinger, bool> &FingerTriggers, TMap<EHandFinger, bool> &FingerBlocked, EHandFinger Finger, AActor* &OverlappedActor, AActor* &AlreadyOverlappedActor, float CurrentGrip);

		bool SetOverlapEnd(TMap<EHandFinger, bool> &FingerOverlapping, EHandFinger Finger);

		void PrintHUD(FString str);

		FString EHandFingerToString(EHandFinger Finger);

	private:
		// Capsule Collider Components
		UPROPERTY(Category = Pawn, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UCapsuleComponent* Thumb_3R;
		UPROPERTY(Category = Pawn, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UCapsuleComponent* Index_3R;
		UPROPERTY(Category = Pawn, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UCapsuleComponent* Middle_3R;
		UPROPERTY(Category = Pawn, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UCapsuleComponent* Ring_3R;
		UPROPERTY(Category = Pawn, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UCapsuleComponent* Pinky_3R;
		UPROPERTY(Category = Pawn, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UCapsuleComponent* Thumb_3L;
		UPROPERTY(Category = Pawn, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UCapsuleComponent* Index_3L;
		UPROPERTY(Category = Pawn, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UCapsuleComponent* Middle_3L;
		UPROPERTY(Category = Pawn, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UCapsuleComponent* Ring_3L;
		UPROPERTY(Category = Pawn, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UCapsuleComponent* Pinky_3L;
		UPROPERTY(Category = Pawn, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UCapsuleComponent* Thumb_2R;
		UPROPERTY(Category = Pawn, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UCapsuleComponent* Index_2R;
		UPROPERTY(Category = Pawn, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UCapsuleComponent* Middle_2R;
		UPROPERTY(Category = Pawn, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UCapsuleComponent* Ring_2R;
		UPROPERTY(Category = Pawn, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UCapsuleComponent* Pinky_2R;
		UPROPERTY(Category = Pawn, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UCapsuleComponent* Thumb_2L;
		UPROPERTY(Category = Pawn, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UCapsuleComponent* Index_2L;
		UPROPERTY(Category = Pawn, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UCapsuleComponent* Middle_2L;
		UPROPERTY(Category = Pawn, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UCapsuleComponent* Ring_2L;
		UCapsuleComponent* Pinky_2L;

	protected:
		UFUNCTION()
		void OnMeshHit(UPrimitiveComponent * HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult & Hit);
		/*UFUNCTION()
		void OnMeshOverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
		UFUNCTION()
		void OnMeshOverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);*/
		/** Capsule Collider Overlap Event Handlers **/
		UFUNCTION()
		void OnThumb_3ROverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
		UFUNCTION()
		void OnThumb_3ROverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
		UFUNCTION()
		void OnIndex_3ROverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
		UFUNCTION()
		void OnIndex_3ROverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
		UFUNCTION()
		void OnMiddle_3ROverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
		UFUNCTION()
		void OnMiddle_3ROverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
		UFUNCTION()
		void OnRing_3ROverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
		UFUNCTION()
		void OnRing_3ROverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
		UFUNCTION()
		void OnPinky_3ROverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
		UFUNCTION()
		void OnPinky_3ROverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
		UFUNCTION()
		void OnThumb_3LOverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
		UFUNCTION()
		void OnThumb_3LOverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
		UFUNCTION()
		void OnIndex_3LOverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
		UFUNCTION()
		void OnIndex_3LOverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
		UFUNCTION()
		void OnMiddle_3LOverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
		UFUNCTION()
		void OnMiddle_3LOverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
		UFUNCTION()
		void OnRing_3LOverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
		UFUNCTION()
		void OnRing_3LOverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
		UFUNCTION()
		void OnPinky_3LOverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
		UFUNCTION()
		void OnPinky_3LOverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
		UFUNCTION()
		void OnThumb_2ROverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
		UFUNCTION()
		void OnThumb_2ROverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
		UFUNCTION()
		void OnIndex_2ROverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
		UFUNCTION()
		void OnIndex_2ROverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
		UFUNCTION()
		void OnMiddle_2ROverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
		UFUNCTION()
		void OnMiddle_2ROverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
		UFUNCTION()
		void OnRing_2ROverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
		UFUNCTION()
		void OnRing_2ROverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
		UFUNCTION()
		void OnPinky_2ROverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
		UFUNCTION()
		void OnPinky_2ROverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
		UFUNCTION()
		void OnThumb_2LOverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
		UFUNCTION()
		void OnThumb_2LOverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
		UFUNCTION()
		void OnIndex_2LOverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
		UFUNCTION()
		void OnIndex_2LOverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
		UFUNCTION()
		void OnMiddle_2LOverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
		UFUNCTION()
		void OnMiddle_2LOverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
		UFUNCTION()
		void OnRing_2LOverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
		UFUNCTION()
		void OnRing_2LOverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
		UFUNCTION()
		void OnPinky_2LOverlapBegin(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
		UFUNCTION()
		void OnPinky_2LOverlapEnd(UPrimitiveComponent * OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};