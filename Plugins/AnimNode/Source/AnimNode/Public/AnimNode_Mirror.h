

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "BoneControllers/AnimNode_ModifyBone.h"
#include "AnimNode_Mirror.generated.h"

class USkeletalMeshComponent;


USTRUCT(BlueprintType)
struct FBonesTransfroms
{
	GENERATED_BODY()
	// Array of names
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BonesTransfroms")
	TMap <FName, FTransform> Map_IdxTransform;
	// We use name for initialisation purposes
};

USTRUCT(BlueprintInternalUseOnly)
struct ANIMNODE_API FAnimNode_Mirror : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()	

	/** Array of bones we will modify**/
	UPROPERTY()
	TArray<FBoneReference> SetOfBonesToModify;

	/** New translation of bone to apply. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinShownByDefault))
	FBonesTransfroms BonesTransfroms;

	/** Whether and how to modify the translation of this bone. */
	UPROPERTY(EditAnywhere, Category = Translation)
	TEnumAsByte<EBoneModificationMode> TranslationMode;

	/** Whether and how to modify the translation of this bone. */
	UPROPERTY(EditAnywhere, Category = Rotation)
	TEnumAsByte<EBoneModificationMode> RotationMode;

	/** Whether and how to modify the translation of this bone. */
	UPROPERTY(EditAnywhere, Category = Scale)
	TEnumAsByte<EBoneModificationMode> ScaleMode;

	/** Reference frame to apply Translation in. */
	UPROPERTY(EditAnywhere, Category = Translation)
	TEnumAsByte<enum EBoneControlSpace> TranslationSpace;

	/** Reference frame to apply Rotation in. */
	UPROPERTY(EditAnywhere, Category = Rotation)
	TEnumAsByte<enum EBoneControlSpace> RotationSpace;

	/** Reference frame to apply Scale in. */
	UPROPERTY(EditAnywhere, Category = Scale)
	TEnumAsByte<enum EBoneControlSpace> ScaleSpace;

	FAnimNode_Mirror();

	// FAnimNode_Base interface
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface

	// FAnimNode_SkeletalControlBase interface
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

private:
	// FAnimNode_SkeletalControlBase interface
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface
};