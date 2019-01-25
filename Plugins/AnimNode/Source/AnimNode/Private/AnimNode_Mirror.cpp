

#include "AnimNode_Mirror.h"
#include "AnimationRuntime.h"
#include "Animation/AnimInstanceProxy.h"

/////////////////////////////////////////////////////
// FAnimNode_ModifyBone

FAnimNode_Mirror::FAnimNode_Mirror()
	: TranslationMode(BMM_Ignore)
	, RotationMode(BMM_Ignore)
	, ScaleMode(BMM_Ignore)
	, TranslationSpace(BCS_ComponentSpace)
	, RotationSpace(BCS_ComponentSpace)
	, ScaleSpace(BCS_ComponentSpace)
{
}

void FAnimNode_Mirror::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);

	DebugLine += "(";
	AddDebugNodeData(DebugLine);
	
	FString setOfBones = "";
	for (auto It = BonesTransfroms.Map_IdxTransform.CreateConstIterator(); It; ++It) {
		setOfBones += It.Key().ToString() + " ";
	}

	DebugLine += FString::Printf(TEXT(" Target: %s)"), *setOfBones);
	DebugData.AddDebugItem(DebugLine);

	ComponentPose.GatherDebugData(DebugData);
}

void FAnimNode_Mirror::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	check(OutBoneTransforms.Num() == 0);
	
	if (BonesTransfroms.Map_IdxTransform.Num() == 0 || SetOfBonesToModify.Num() == 0)
		return;

	// the way we apply transform is same as FMatrix or FTransform
	// we apply scale first, and rotation, and translation
	// if you'd like to translate first, you'll need two nodes that first node does translate and second nodes to rotate.
	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();

	for (auto& b_m : SetOfBonesToModify)
	{
		FCompactPoseBoneIndex CompactPoseBoneToModify = b_m.GetCompactPoseIndex(BoneContainer);
		FTransform NewBoneTM = Output.Pose.GetComponentSpaceTransform(CompactPoseBoneToModify);
		FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();

		FTransform cachedTransform(*BonesTransfroms.Map_IdxTransform.Find(b_m.BoneName));
		
		if (ScaleMode != BMM_Ignore)
		{
			// Convert to Bone Space.
			FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, NewBoneTM, CompactPoseBoneToModify, ScaleSpace);

			if (ScaleMode == BMM_Additive)
			{
				NewBoneTM.SetScale3D(NewBoneTM.GetScale3D() * (cachedTransform.GetScale3D()));
			}
			else
			{
				NewBoneTM.SetScale3D(cachedTransform.GetScale3D());
			}

			// Convert back to Component Space.
			FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, NewBoneTM, CompactPoseBoneToModify, ScaleSpace);
		}

		if (RotationMode != BMM_Ignore)
		{
			// Convert to Bone Space.
			FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, NewBoneTM, CompactPoseBoneToModify, RotationSpace);

			const FQuat BoneQuat(cachedTransform.GetRotation());
			if (RotationMode == BMM_Additive)
			{
				NewBoneTM.SetRotation(BoneQuat * NewBoneTM.GetRotation());
			}
			else
			{
				NewBoneTM.SetRotation(BoneQuat);
			}

			// Convert back to Component Space.
			FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, NewBoneTM, CompactPoseBoneToModify, RotationSpace);
		}

		if (TranslationMode != BMM_Ignore)
		{
			// Convert to Bone Space.
			FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, NewBoneTM, CompactPoseBoneToModify, TranslationSpace);

			if (TranslationMode == BMM_Additive)
			{
				NewBoneTM.AddToTranslation(cachedTransform.GetTranslation());
			}
			else
			{
				NewBoneTM.SetTranslation(cachedTransform.GetTranslation());
			}

			// Convert back to Component Space.
			FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, NewBoneTM, CompactPoseBoneToModify, TranslationSpace);
		}

		OutBoneTransforms.Add(FBoneTransform(b_m.GetCompactPoseIndex(BoneContainer), NewBoneTM));
	}
}

bool FAnimNode_Mirror::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	for (auto& b_m : SetOfBonesToModify) {
		if (!b_m.IsValidToEvaluate(RequiredBones))
			return false;
	}
	return true;
}

void FAnimNode_Mirror::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	for (auto It = BonesTransfroms.Map_IdxTransform.CreateConstIterator(); It; ++It) {
		FBoneReference br(It.Key());
		br.Initialize(RequiredBones);
		if (br.BoneIndex != INDEX_NONE)
			SetOfBonesToModify.AddUnique(br);
	}
}
