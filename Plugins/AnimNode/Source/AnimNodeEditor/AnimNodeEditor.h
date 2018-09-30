#pragma once
 
//Use for plugin

#include "CoreMinimal.h"
#include "ModuleManager.h"

class FAnimNodeEditorModule : public IModuleInterface
{
public:

	// IModuleInterface implementation 
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};


//Use for module
//#include "ModuleManager.h"