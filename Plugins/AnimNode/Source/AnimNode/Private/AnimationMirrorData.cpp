/*
	By fBlah (Ajit D'Monte)
*/

#include "AnimationMirrorData.h" 

#include <EngineGlobals.h>
#include <Runtime/Engine/Classes/Engine/Engine.h>

UAnimationMirrorData::UAnimationMirrorData()
{
	PelvisBoneName = "Pelvis";
}

void UAnimationMirrorData::SetMirrorMappedBone(const FName bone_name, const FName mirror_bone_name, const MirrorDir mirror_axis, const MirrorDir right_axis)
{	
	mMirrorData.Emplace(bone_name);
	mMirrorData.Emplace(mirror_bone_name);
	mMirrorAxisData.Emplace(mirror_axis);
	mRightAxisData.Emplace(mirror_axis);
}


void UAnimationMirrorData::SetMirrorMappedData(TArray<FName> pMirrorData, TArray<MirrorDir> pMirrorAxisData, TArray<MirrorDir> pRightAxisData)
{
	mMirrorData.Empty();	
	mMirrorAxisData.Empty();
	mRightAxisData.Empty();
	for (int32 i = 0; i < pMirrorData.Num(); ++i) {
		mMirrorData.Emplace(pMirrorData[i]);		
	};
	for (int32 i = 0; i < pMirrorAxisData.Num(); ++i) {
		mMirrorAxisData.Emplace(pMirrorAxisData[i]);
	};
	for (int32 i = 0; i < pRightAxisData.Num(); ++i) {
		mRightAxisData.Emplace(pRightAxisData[i]);
	};	
}

void UAnimationMirrorData::SetPelvisBoneName(const FName bone_name)
{
	PelvisBoneName = bone_name;
}

FName UAnimationMirrorData::GetPelvisBoneName() const
{
	return PelvisBoneName;
}

FName  UAnimationMirrorData::GetMirrorMappedBone(const FName bone_name) const
{
	int32 Index;
	if (mMirrorData.Find(bone_name, Index))
	{
		if (Index % 2 == 0) {
			return mMirrorData[Index + 1];
		}
		else
		{
			return mMirrorData[Index - 1];
		}
	}
	else
	{
		return FName("None");
	}
}

TArray<FName> UAnimationMirrorData::GetBoneMirrorDataStructure() const
{
	return mMirrorData;
}

TArray<MirrorDir> UAnimationMirrorData::GetBoneMirrorAxisDataStructure() const
{
	return mMirrorAxisData;
}

TArray<MirrorDir> UAnimationMirrorData::GetBoneRightAxisDataStructure() const
{
	return mRightAxisData;
}