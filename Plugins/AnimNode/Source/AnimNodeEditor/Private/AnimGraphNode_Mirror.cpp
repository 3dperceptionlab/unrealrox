#include "AnimGraphNode_Mirror.h"
#include "UnrealWidget.h"
#include "AnimNodeEditModes.h"
#include "Kismet2/CompilerResultsLog.h"


/////////////////////////////////////////////////////
// UAnimGraphNode_ModifyBone

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_Mirror::UAnimGraphNode_Mirror(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CurWidgetMode = (int32)FWidget::WM_Rotate;
}

void UAnimGraphNode_Mirror::ValidateAnimNodeDuringCompilation(USkeleton* ForSkeleton, FCompilerResultsLog& MessageLog)
{
	for (auto It = Node.BonesTransfroms.Map_IdxTransform.CreateConstIterator(); It; ++It) {
		// Temporary fix where skeleton is not fully loaded during AnimBP compilation and thus virtual bone name check is invalid UE-39499 (NEED FIX) 
		if (ForSkeleton && !ForSkeleton->HasAnyFlags(RF_NeedPostLoad))
		{
			if (ForSkeleton->GetReferenceSkeleton().FindBoneIndex(It.Key()) == INDEX_NONE)
			{
				if (It.Key() == NAME_None)
				{
					MessageLog.Warning(*LOCTEXT("NoBoneSelectedToModify", "@@ - You must pick a bone to modify").ToString(), this);
				}
				else
				{
					FFormatNamedArguments Args;
					Args.Add(TEXT("BoneName"), FText::FromName(It.Key()));

					FText Msg = FText::Format(LOCTEXT("NoBoneFoundToModify", "@@ - Bone {BoneName} not found in Skeleton"), Args);

					MessageLog.Warning(*Msg.ToString(), this);
				}
			}
		}
		if ((Node.TranslationMode == BMM_Ignore) && (Node.RotationMode == BMM_Ignore) && (Node.ScaleMode == BMM_Ignore))
		{
			MessageLog.Warning(*LOCTEXT("NothingToModify", "@@ - No components to modify selected.  Either Rotation, Translation, or Scale should be set to something other than Ignore").ToString(), this);
		}
		Super::ValidateAnimNodeDuringCompilation(ForSkeleton, MessageLog);
	}
}

FText UAnimGraphNode_Mirror::GetControllerDescription() const
{
	return LOCTEXT("BulkTransformModifyBones", "Bulk Transform (Modify) Bones");
}

FText UAnimGraphNode_Mirror::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_Mirror_Tooltip", "The Transform Bone node alters the transform - i.e. Translation, Rotation, or Scale - of the bone");
}

FText UAnimGraphNode_Mirror::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if ((TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle) && (Node.SetOfBonesToModify.Num() == 0))
	{
		return GetControllerDescription();
	}
	// @TODO: the bone can be altered in the property editor, so we have to 
	//        choose to mark this dirty when that happens for this to properly work
	else //if (!CachedNodeTitles.IsTitleCached(TitleType, this))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ControllerDescription"), GetControllerDescription());
		
		FString s_bones("");

		if (Node.BonesTransfroms.Map_IdxTransform.Num() > 3) {
			s_bones = "Multiple bones";
		} else if (Node.BonesTransfroms.Map_IdxTransform.Num() != 0) {
			for (auto& elem : Node.BonesTransfroms.Map_IdxTransform) {
				s_bones += elem.Key.ToString() + " ";
			}
		} else {
			s_bones = "None or coming from variable";
		}
		
		Args.Add(TEXT("BoneName"), FText::FromString(s_bones));
		
		// FText::Format() is slow, so we cache this to save on performance
		if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
		{
			CachedNodeTitles.SetCachedTitle(TitleType, FText::Format(LOCTEXT("AnimGraphNode_Mirror_ListTitle", "{ControllerDescription} - Bone: {BoneName}"), Args), this);
		}
		else
		{
			CachedNodeTitles.SetCachedTitle(TitleType, FText::Format(LOCTEXT("AnimGraphNode_Mirror_Title", "{ControllerDescription}\nBone: {BoneName}"), Args), this);
		}
	}
	return CachedNodeTitles[TitleType];
}

void UAnimGraphNode_Mirror::CopyNodeDataToPreviewNode(FAnimNode_Base* InPreviewNode)
{
	FAnimNode_Mirror* ModifyBone = static_cast<FAnimNode_Mirror*>(InPreviewNode);

	// copies Pin values from the internal node to get data which are not compiled yet
	// ModifyBone->SetOfBonesToModify = Node.SetOfBonesToModify;
	for (auto &a : Node.SetOfBonesToModify) {
		ModifyBone->SetOfBonesToModify.AddUnique(a);
	}
	// ModifyBone->BonesTransfroms.Map_IdxTransform = Node.BonesTransfroms.Map_IdxTransform;
	for (auto &a : Node.BonesTransfroms.Map_IdxTransform) {
		ModifyBone->BonesTransfroms.Map_IdxTransform.Emplace(a.Key, a.Value);
	}

	// copies Modes
	ModifyBone->TranslationMode = Node.TranslationMode;
	ModifyBone->RotationMode = Node.RotationMode;
	ModifyBone->ScaleMode = Node.ScaleMode;

	// copies Spaces
	ModifyBone->TranslationSpace = Node.TranslationSpace;
	ModifyBone->RotationSpace = Node.RotationSpace;
	ModifyBone->ScaleSpace = Node.ScaleSpace;
}

FEditorModeID UAnimGraphNode_Mirror::GetEditorMode() const
{
	return AnimNodeEditModes::AnimNode;
}

void UAnimGraphNode_Mirror::CopyPinDefaultsToNodeData(UEdGraphPin* InPin)
{ }

#undef LOCTEXT_NAMESPACE
