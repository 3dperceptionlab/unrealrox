#include "ROXObjectPainter.h"
#include "SceneViewport.h"
#include "Engine/StaticMesh.h"
#include "RenderResource.h"
#include "SharedPointer.h"
#include "Components/StaticMeshComponent.h"
#include "FileHelper.h"

FROXObjectPainter& FROXObjectPainter::Get()
{
	static FROXObjectPainter Singleton;
	return Singleton;
}

/** Utility function to generate color map */
int32 GetChannelValue(uint32 Index)
{
	static int32 Values[256] = { 0 };
	static bool Init = false;
	if (!Init)
	{
		float Step = 256;
		uint32 Iter = 0;
		Values[0] = 0;
		while (Step >= 1)
		{
			for (uint32 Value = Step - 1; Value <= 256; Value += Step * 2)
			{
				Iter++;
				Values[Iter] = Value;
			}
			Step /= 2;
		}
		Init = true;
	}
	if (Index >= 0 && Index <= 255)
	{
		return Values[Index];
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid channel index"));
		return -1;
	}
}

void GetColors(int32 MaxVal, bool Fix1, bool Fix2, bool Fix3, TArray<FColor>& ColorMap)
{
	for (int32 I = 0; I <= (Fix1 ? 0 : MaxVal - 1); I++)
	{
		for (int32 J = 0; J <= (Fix2 ? 0 : MaxVal - 1); J++)
		{
			for (int32 K = 0; K <= (Fix3 ? 0 : MaxVal - 1); K++)
			{
				uint8 R = GetChannelValue(Fix1 ? MaxVal : I);
				uint8 G = GetChannelValue(Fix2 ? MaxVal : J);
				uint8 B = GetChannelValue(Fix3 ? MaxVal : K);
				FColor Color(R, G, B, 255);
				if (Color != FColor::Black)
				{
					ColorMap.Add(Color);
				}
			}
		}
	}
}

/** TODO: support more than 1000 objects */
FColor GetColorFromColorMap(int32 ObjectIndex)
{
	static TArray<FColor> ColorMap;
	int NumPerChannel = 32;
	if (ColorMap.Num() == 0)
	{
		// 32 ^ 3
		for (int32 MaxChannelIndex = 0; MaxChannelIndex < NumPerChannel; MaxChannelIndex++) // Get color map for 1000 objects
		{
			// GetColors(MaxChannelIndex, false, false, false, ColorMap);
			GetColors(MaxChannelIndex, false, false, true, ColorMap);
			GetColors(MaxChannelIndex, false, true, false, ColorMap);
			GetColors(MaxChannelIndex, false, true, true, ColorMap);
			GetColors(MaxChannelIndex, true, false, false, ColorMap);
			GetColors(MaxChannelIndex, true, false, true, ColorMap);
			GetColors(MaxChannelIndex, true, true, false, ColorMap);
			GetColors(MaxChannelIndex, true, true, true, ColorMap);
		}
	}
	if (ObjectIndex < 0 || ObjectIndex >= pow(NumPerChannel, 3))
	{
		UE_LOG(LogTemp, Error, TEXT("Object index %d is out of the color map boundary [%d, %d]"), ObjectIndex, 0, pow(NumPerChannel, 3));
	}
	return ColorMap[ObjectIndex];
}

/** Check whether an actor can be painted with vertex color */
bool IsPaintable(AActor* Actor)
{
	TArray<UMeshComponent*> PaintableComponents;
	Actor->GetComponents<UMeshComponent>(PaintableComponents);
	if (PaintableComponents.Num() == 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

AActor* FROXObjectPainter::GetObject(FString ActorId)
{
	/** Return the pointer of an object, return NULL if object not found */
	if (Id2Actor.Contains(ActorId))
	{
		return Id2Actor[ActorId];
	}
	else
	{
		return NULL;
	}
}

TSharedPtr<FJsonObject> ColorToJson(FColor color)
{
	TSharedPtr<FJsonObject> JsonObject_Color = MakeShareable(new FJsonObject());
	JsonObject_Color->SetNumberField("r", color.R);
	JsonObject_Color->SetNumberField("g", color.G);
	JsonObject_Color->SetNumberField("b", color.B);
	return JsonObject_Color;
}

bool FROXObjectPainter::PrintToJson(FString filename)
{
	bool printed = false;

	if (Id2Actor.Num() > 0 && Id2Color.Num() > 0 && Id2Actor.Num() == Id2Color.Num())
	{
		TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
		TArray<TSharedPtr<FJsonValue>> JsonArray_SceneObjects;

		TArray<FString> ActorIds;
		Id2Color.GetKeys(ActorIds);

		for (FString ActorId : ActorIds)
		{
			FColor ActorColor = *Id2Color.Find(ActorId);

			TSharedPtr<FJsonObject> JsonObject_SceneObject = MakeShareable(new FJsonObject());
			JsonObject_SceneObject->SetStringField("instance_name", ActorId);
			JsonObject_SceneObject->SetObjectField("instance_color", ColorToJson(ActorColor));
			JsonObject_SceneObject->SetStringField("class", "none");

			JsonArray_SceneObjects.Add(MakeShareable(new FJsonValueObject(JsonObject_SceneObject)));
		}
		JsonObject->SetArrayField("SceneObjects", JsonArray_SceneObjects);

		// Write JSON file
		FString OutputString;
		TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
		FFileHelper::SaveStringToFile(OutputString, *filename, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), EFileWrite::FILEWRITE_None);
		printed = true;
	}

	return printed;
}

void FROXObjectPainter::Reset(ULevel* InLevel)
{
	this->Level = InLevel;
	this->Id2Color.Empty();
	this->Id2Actor.Empty();

	// This list needs to be generated everytime the game restarted.
	check(Level);

	uint32 ObjectIndex = 0;
	for (AActor* Actor : Level->Actors)
	{
		if (Actor && IsPaintable(Actor))
		{
			FString ActorId = Actor->GetHumanReadableName();
			Id2Actor.Emplace(ActorId, Actor);
			FColor NewColor = GetColorFromColorMap(ObjectIndex);
			Id2Color.Emplace(ActorId, NewColor);
			ObjectIndex++;
		}
	}

	for (auto& Elem : Id2Color)
	{
		FString ActorId = Elem.Key;
		FColor NewColor = Elem.Value;
		AActor* Actor = Id2Actor[ActorId];
		check(PaintObject(Actor, NewColor, false));
	}
}

/** DisplayColor is the color that the screen will show
If DisplayColor.R = 128, the display will show 0.5 voltage
To achieve this, UnrealEngine will do gamma correction.
The value on image will be 187.
https://en.wikipedia.org/wiki/Gamma_correction#Methods_to_perform_display_gamma_correction_in_computing
*/
bool FROXObjectPainter::PaintObject(AActor* Actor, const FColor& Color, bool IsColorGammaEncoded)
{
	if (!Actor) return false;

	FColor NewColor;
	if (IsColorGammaEncoded)
	{
		FLinearColor LinearColor = FLinearColor::FromPow22Color(Color);
		NewColor = LinearColor.ToFColor(false);
	}
	else
	{
		NewColor = Color;
	}

	TArray<UMeshComponent*> PaintableComponents;
	Actor->GetComponents<UMeshComponent>(PaintableComponents);


	for (auto MeshComponent : PaintableComponents)
	{
		if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(MeshComponent))
		{
			UStaticMesh* StaticMesh;
			StaticMesh = StaticMeshComponent->GetStaticMesh(); // This is a new function introduced in 4.14

			if (StaticMesh)
			{
				uint32 NumLODLevel = StaticMesh->RenderData->LODResources.Num();
				for (uint32 PaintingMeshLODIndex = 0; PaintingMeshLODIndex < NumLODLevel; PaintingMeshLODIndex++)
				{
					FStaticMeshLODResources& LODModel = StaticMesh->RenderData->LODResources[PaintingMeshLODIndex];
					FStaticMeshComponentLODInfo* InstanceMeshLODInfo = NULL;

					// PaintingMeshLODIndex + 1 is the minimum requirement, enlarge if not satisfied
					StaticMeshComponent->SetLODDataCount(PaintingMeshLODIndex + 1, StaticMeshComponent->LODData.Num());
					InstanceMeshLODInfo = &StaticMeshComponent->LODData[PaintingMeshLODIndex];

					InstanceMeshLODInfo->ReleaseOverrideVertexColorsAndBlock();
					// Setup OverrideVertexColors
					// if (!InstanceMeshLODInfo->OverrideVertexColors) // TODO: Check this
					{
						InstanceMeshLODInfo->OverrideVertexColors = new FColorVertexBuffer;

						FColor FillColor = FColor(255, 255, 255, 255);
						InstanceMeshLODInfo->OverrideVertexColors->InitFromSingleColor(FColor::White, LODModel.GetNumVertices());
					}

					uint32 NumVertices = LODModel.GetNumVertices();
					check(InstanceMeshLODInfo->OverrideVertexColors);
					check(NumVertices <= InstanceMeshLODInfo->OverrideVertexColors->GetNumVertices());
					// StaticMeshComponent->CachePaintedDataIfNecessary();

					for (uint32 ColorIndex = 0; ColorIndex < NumVertices; ++ColorIndex)
					{
						// LODModel.ColorVertexBuffer.VertexColor(ColorIndex) = NewColor;  // This is vertex level
						// Need to initialize the vertex buffer first
						uint32 NumOverrideVertexColors = InstanceMeshLODInfo->OverrideVertexColors->GetNumVertices();
						uint32 NumPaintedVertices = InstanceMeshLODInfo->PaintedVertices.Num();
						// check(NumOverrideVertexColors == NumPaintedVertices);
						InstanceMeshLODInfo->OverrideVertexColors->VertexColor(ColorIndex) = NewColor;
						// InstanceMeshLODInfo->PaintedVertices[ColorIndex].Color = NewColor;
					}
					BeginInitResource(InstanceMeshLODInfo->OverrideVertexColors);
					StaticMeshComponent->MarkRenderStateDirty();
					// BeginUpdateResourceRHI(InstanceMeshLODInfo->OverrideVertexColors);
				}
			}
		}
	}
	return true;
}
