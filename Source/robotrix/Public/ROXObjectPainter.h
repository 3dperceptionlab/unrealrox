#pragma once

//#include "ExecStatus.h"
/*
* Annotate objects in the scene with a unique color
* Used to paint vertex color
*/
class ROBOTRIX_API FROXObjectPainter
{
private:
	/** The level this ObjectPainter associated with */
	ULevel* Level;

	FROXObjectPainter() {}
	/** The assigned color for each object */
	TMap<FString, FColor> Id2Color;
	/** A list of paintable objects */
	TMap<FString, AActor*> Id2Actor;

public:
	/** Return the singleton of FObjectPainter */
	static FROXObjectPainter& Get();

	/** Reset this to uninitialized state */
	void Reset(ULevel* InLevel);

	/** Vertex paint one object with Flood-Fill */
	bool PaintObject(AActor* Actor, const FColor& Color, bool IsColorGammaEncoded = true);

	/** Get a pointer to an object */
	AActor* GetObject(FString ObjectName);

	/** Print color mapping to JSON file */
	bool PrintToJson(FString filename);
};
