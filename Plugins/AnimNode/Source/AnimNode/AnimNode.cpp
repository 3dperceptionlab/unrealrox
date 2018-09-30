#include "AnimNode.h"


//Use for plugin

#define LOCTEXT_NAMESPACE "FAnimNodeModule"

void FAnimNodeModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FAnimNodeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAnimNodeModule, AnimNode)


//Use this for game module

//IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, AnimNode, "AnimNode");