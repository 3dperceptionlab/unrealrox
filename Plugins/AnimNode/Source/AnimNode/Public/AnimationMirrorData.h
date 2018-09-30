/*
    By fBlah (Ajit D'Monte)
*/

// I used the code on this site:
// http://peyman-mass.blogspot.in/2015/11/mirroring-3d-character-animations.html
#pragma once

#include "CoreMinimal.h"
#include "Object.h"

#include "AnimationMirrorData.generated.h"

/**
*
*/
UENUM(BlueprintType)
enum class MirrorDir : uint8
{
	None = 0,
	X_Axis = 1,
	Y_Axis = 2,
	Z_Axis = 3,
	XY_Axis = 4,
	YZ_Axis = 5,
	XZ_Axis = 6,
};

UCLASS(BlueprintType)
class ANIMNODE_API UAnimationMirrorData : public UObject
{
	GENERATED_BODY()
public:

	UAnimationMirrorData();

	//Shows mirror axis. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mirror Animation")
		MirrorDir MirrorAxis_Rot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mirror Animation")
		MirrorDir RightAxis;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mirror Animation")
		MirrorDir PelvisMirrorAxis_Rot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mirror Animation")
		MirrorDir PelvisRightAxis;

	//Functions  
	UFUNCTION(BlueprintCallable, Category = "Mirror Animation")
		void SetMirrorMappedBone(const FName bone_name, const FName mirror_bone_name, const MirrorDir mirror_axis, const MirrorDir right_axis);
	
	UFUNCTION(BlueprintCallable, Category = "Mirror Animation")
		void SetMirrorMappedData(TArray<FName> pMirrorData, TArray<MirrorDir> pMirrorAxisData, TArray<MirrorDir> pRightAxisData);

	UFUNCTION(BlueprintCallable, Category = "Mirror Animation")
		void SetPelvisBoneName(const FName bone_name);

	UFUNCTION(BlueprintCallable, Category = "Mirror Animation")
		FName GetPelvisBoneName() const;

	UFUNCTION(BlueprintCallable, Category = "Mirror Animation")
		FName  GetMirrorMappedBone(const FName bone_name) const;

	UFUNCTION(BlueprintCallable, Category = "Mirror Animation")
		TArray<FName> GetBoneMirrorDataStructure() const;
	UFUNCTION(BlueprintCallable, Category = "Mirror Animation")
		TArray<MirrorDir> GetBoneMirrorAxisDataStructure() const;
	UFUNCTION(BlueprintCallable, Category = "Mirror Animation")
		TArray<MirrorDir> GetBoneRightAxisDataStructure() const;

protected:
	TArray<FName> mMirrorData;
	TArray<MirrorDir> mMirrorAxisData;
	TArray<MirrorDir> mRightAxisData;
	FName PelvisBoneName;
};