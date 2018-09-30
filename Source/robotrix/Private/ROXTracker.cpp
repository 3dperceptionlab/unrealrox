// Copyright 2018, 3D Perception Lab

#include "ROXTracker.h"
#include "DateTime.h"
#include "TimerManager.h"
#include "Engine/PostProcessVolume.h"
#include "ROXObjectPainter.h"
#include "ROXTypes.h"
#include "CommandLine.h"

// Sets default values
AROXTracker::AROXTracker() :
	bIsRecording(false),
	bRecordMode(true),
	bStandaloneMode(false),
	Persistence_Level_Filter_Str("UEDPIE_0"),
	bDebugMode(false),
	initial_delay(2.0f),
	place_cameras_delay(.1f),
	first_viewmode_of_frame_delay(.1f),
	change_viewmode_delay(.2f),
	take_screenshot_delay(.1f),
	scene_folder("SceneText"),
	screenshots_folder("Screenshots"),
	scene_file_name_prefix("scene"),
	input_scene_TXT_file_name("scene"),
	output_scene_json_file_name("scene"),
	generate_rgb(true),
	format_rgb(EROXRGBImageFormats::RIF_JPG95),
	generate_depth(true),
	generate_object_mask(true),
	generate_normal(true),
	generate_depth_txt_cm(false),
	screenshot_width(1920),
	screenshot_height(1080),
	frame_status_output_period(100),
	fileHeaderWritten(false),
	numFrame(0),
	CurrentViewmode(EROXViewMode_First),
	CurrentViewTarget(0),
	CurrentCamRebuildMode(0),
	CurrentJsonFile(0),
	JsonReadStartTime(0),
	LastFrameTime(0)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = bRecordMode;
	PrimaryActorTick.bStartWithTickEnabled = true;

	DepthMat = nullptr;
	static ConstructorHelpers::FObjectFinder<UMaterial> matDepth(TEXT("/Game/Common/ViewModeMats/SceneDepth.SceneDepth"));
	if (matDepth.Succeeded())
	{
		DepthMat = (UMaterial*)matDepth.Object;
	}

	DepthWUMat = nullptr;
	static ConstructorHelpers::FObjectFinder<UMaterial> matDepthWU(TEXT("/Game/Common/ViewModeMats/SceneDepthWorldUnits.SceneDepthWorldUnits"));
	if (matDepthWU.Succeeded())
	{
		DepthWUMat = (UMaterial*)matDepthWU.Object;
	}

	DepthCmMat = nullptr;
	static ConstructorHelpers::FObjectFinder<UMaterial> matDepthCm(TEXT("/Game/Common/ViewModeMats/SceneDepth_CmToGray.SceneDepth_CmToGray"));
	if (matDepthCm.Succeeded())
	{
		DepthCmMat = (UMaterial*)matDepthCm.Object;
	}

	NormalMat = nullptr;
	static ConstructorHelpers::FObjectFinder<UMaterial> matNormal(TEXT("/Game/Common/ViewModeMats/WorldNormal.WorldNormal"));
	if (matNormal.Succeeded())
	{
		NormalMat = (UMaterial*)matNormal.Object;
	}

	DepthTextureRenderer = nullptr;
	static ConstructorHelpers::FObjectFinder<UTextureRenderTarget2D> sceneCapture(TEXT("/Game/Common/ViewModeMats/RT_SceneDepth.RT_SceneDepth"));
	if (sceneCapture.Succeeded())
	{
		DepthTextureRenderer = (UTextureRenderTarget2D*)sceneCapture.Object;
	}

	json_file_names.Add("scene");
	start_frames.Add(0);
	scene_save_directory = FPaths::ProjectUserDir();
	screenshots_save_directory = FPaths::ProjectUserDir();
	absolute_file_path = scene_save_directory + scene_folder + "/" + scene_file_name_prefix + ".txt";
}

// Called when the game starts or when spawned
void AROXTracker::BeginPlay()
{
	Super::BeginPlay();

	// PPX init
	GameShowFlags = new FEngineShowFlags(GetWorld()->GetGameViewport()->EngineShowFlags);
	FROXObjectPainter::Get().Reset(GetLevel());
	FEngineShowFlags ShowFlags = GetWorld()->GetGameViewport()->EngineShowFlags;

	//screenshot resolution
	GScreenshotResolutionX = screenshot_width; // 1920  1280
	GScreenshotResolutionY = screenshot_height;  // 1080  720

	for (AROXBasePawn* pawn : Pawns)
	{
		pawn->InitFromTracker(bRecordMode, bDebugMode, this);
		ViewTargets.Add(pawn);

		if (pawn->GetController())
		{
			ControllerPawn = pawn;
			ControllerPawn->isRecordMode();
		}
	}

	for (ACameraActor* Cam : CameraActors)
	{
		ViewTargets.Add(Cam);
	}

	EROXViewModeList.Empty();
	if (generate_rgb) EROXViewModeList.Add(EROXViewMode::RVM_Lit);
	if (generate_depth || generate_depth_txt_cm) EROXViewModeList.Add(EROXViewMode::RVM_Depth);
	if (generate_object_mask) EROXViewModeList.Add(EROXViewMode::RVM_ObjectMask);
	if (generate_normal) EROXViewModeList.Add(EROXViewMode::RVM_Normal);

	if (EROXViewModeList.Num() == 0) EROXViewModeList.Add(EROXViewMode::RVM_Lit);
	EROXViewMode_First = EROXViewModeList[0];
	EROXViewMode_Last = EROXViewModeList.Last();


	if (!bRecordMode)
	{
		// Spawn SceneCapture2D for capture Depth data
		FActorSpawnParameters ActorSpawnParams;
		ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ActorSpawnParams.bDeferConstruction = true;
		ActorSpawnParams.Name = FName("SceneCaptureDepth");
		SceneCapture_depth = GetWorld()->SpawnActor<ASceneCapture2D>(ASceneCapture2D::StaticClass(), ActorSpawnParams);
		SceneCapture_depth->GetCaptureComponent2D()->TextureTarget = DepthTextureRenderer;
		SceneCapture_depth->GetCaptureComponent2D()->CaptureSource = ESceneCaptureSource::SCS_SceneDepth;

		
		while (json_file_names.Num() > start_frames.Num())
		{
			start_frames.Add(0);
		}

		RebuildModeBegin();
	}
	else
	{
		PrintInstanceClassJson();
	}
}

void AROXTracker::PrintInstanceClassJson()
{
	FString instance_class_json;
	FString instance_class_json_path = scene_save_directory + scene_folder + "/instance_class.json";
	bool file_found = FFileHelper::LoadFileToString(instance_class_json, *instance_class_json_path);
	if (!file_found)
	{
		TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

		for (TObjectIterator<AStaticMeshActor> Itr; Itr; ++Itr)
		{
			FString fullName = Itr->GetFullName();
			if (bStandaloneMode || fullName.Contains(Persistence_Level_Filter_Str))
			{
				JsonObject->SetStringField(Itr->GetName(), "none");
			}
		}

		// Write JSON file
		instance_class_json = "";
		TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&instance_class_json);
		FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
		FFileHelper::SaveStringToFile(instance_class_json, *instance_class_json_path, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), EFileWrite::FILEWRITE_Append);

		FString success_message("JSON file instance_class.json has been created successfully.");
		UE_LOG(LogTemp, Warning, TEXT("%s"), *success_message);
	}
	else
	{
		FString error_message("JSON file instance_class.json already exists. Please, check and remove or rename that file in order to create a new one.");
		UE_LOG(LogTemp, Warning, TEXT("%s"), *error_message);
	}
}

// Called every frame
void AROXTracker::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GEngine)
	{
		if (bRecordMode && bIsRecording)
		{
			WriteScene();
		}
	}
}

void AROXTracker::WriteHeader()
{
	//Camera Info
	FString begin_string_ = "Cameras " + FString::FromInt(CameraActors.Num()) + "\r\n";
	for (ACameraActor* CameraActor : CameraActors)
	{
		/* TODO: stereo dist and fov will be part of the functionality added in a near future */
		float stereo_dist(0.0f);
		float field_of_view(CameraActor->GetCameraComponent()->FieldOfView);
		/* TODO: stereo dist and fov will be part of the functionality added in a near future */
		begin_string_ += CameraActor->GetName() + " " + FString::SanitizeFloat(stereo_dist) + " " + FString::SanitizeFloat(field_of_view) + "\r\n";
	}

	// Movable StaticMeshActor dump
	CacheStaticMeshActors();
	begin_string_ += "Objects " + FString::FromInt(CachedSM.Num()) + "\r\n";

	// Pawns dump
	FString skeletaldump("");
	for (AROXBasePawn* rbp : Pawns)
	{
		skeletaldump += rbp->GetActorLabel() + " " + FString::FromInt(rbp->GetMeshComponent()->GetAllSocketNames().Num()) + "\r\n";
	}
	begin_string_ += "Skeletons " + FString::FromInt(Pawns.Num()) + "\r\n" + skeletaldump;

	// Non-movable StaticMeshActor dump
	FString nonmovabledump("");
	int n_nonmovable = 0;
	for (TObjectIterator<AStaticMeshActor> Itr; Itr; ++Itr)
	{
		FString fullName = Itr->GetFullName();
		if (!Itr->ActorHasTag(FName("trojan")) && (fullName.Contains(Persistence_Level_Filter_Str) || bStandaloneMode || bDebugMode) && Itr->GetStaticMeshComponent()->Mobility != EComponentMobility::Movable)
		{
			n_nonmovable++;
			FString actor_name_ = Itr->GetName();
			FString actor_full_name_ = Itr->GetFullName();
			FVector actor_location_ = Itr->GetActorLocation();
			FRotator actor_rotation_ = Itr->GetActorRotation();

			nonmovabledump += actor_name_ + " " + actor_location_.ToString() + " " + actor_rotation_.ToString()
				+ " MIN:" + Itr->GetComponentsBoundingBox(true).Min.ToString() + " MAX:" + Itr->GetComponentsBoundingBox(true).Max.ToString() +
				((bDebugMode) ? (" " + actor_full_name_ + "\r\n") : "\r\n");
		}
	}
	begin_string_ += "NonMovableObjects " + FString::FromInt(n_nonmovable) + "\r\n" + nonmovabledump;


	(new FAutoDeleteAsyncTask<FWriteStringTask>(begin_string_, absolute_file_path))->StartBackgroundTask();
}

void AROXTracker::WriteScene()
{
	//GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, "Recording..");
	FString tick_string_ = "frame\r\n";
	float time_ = UGameplayStatics::GetRealTimeSeconds(GetWorld());
	FString time_string_ = FString::SanitizeFloat(time_ * 1000.0f);
	tick_string_ += FString::FromInt(numFrame) + " " + time_string_ + "\r\n";
	numFrame++;

	// Camera dump
	FString tick_string_cam_aux_ = "";
	int n_cameras = 0;
	for (ACameraActor* CameraActor : CameraActors)
	{
		if (IsValid(CameraActor))
		{
			FString camera_name_ = CameraActor->GetName();
			FVector camera_location_ = CameraActor->GetActorLocation();
			FRotator camera_rotation_ = CameraActor->GetActorRotation();

			FString camera_string_ = camera_name_ + " " + camera_location_.ToString() + " " + camera_rotation_.ToString();
			//FString camera_string_ = "Camera " + camera_location_.ToString() + " " + camera_rotation_.ToString();
			if (bDebugMode)
			{
				camera_string_ += " " + CameraActor->GetFullName();
			}
			tick_string_cam_aux_ += camera_string_ + "\r\n";
		}
	}
	//tick_string_ += "Cameras " + FString::FromInt(n_cameras) + "\r\n" + tick_string_cam_aux_;
	tick_string_ += tick_string_cam_aux_;



	FString ObjectsString("objects\r\n");
	FString SkeletonsString("skeletons\r\n");

	// We cannot assume all the pawns will have a skeleton, only those inheriting our type will
	for (auto rbp : Pawns)
	{
		// Same as with the Object Iterator, access the subclass instance with the * or -> operators.
		TArray<FName> sckt_name = rbp->GetMeshComponent()->GetAllSocketNames();

		SkeletonsString += rbp->GetActorLabel() + " " + rbp->GetActorLocation().ToString() + " " +
			rbp->GetActorRotation().ToString() + "\r\n";

		for (FName scktnm : sckt_name)
		{
			FTransform sckttrans(rbp->GetMeshComponent()->GetSocketTransform(scktnm));
			SkeletonsString += scktnm.ToString() + " " + sckttrans.GetLocation().ToString() + " " + sckttrans.Rotator().ToString() +
				+" MIN:" + FVector::ZeroVector.ToString() + " MAX:" + FVector::ZeroVector.ToString() + "\r\n";
		}
	}

	// StaticMeshActor dump
	for (auto Itr : CachedSM)
	{
		FString actor_name_ = Itr->GetName();
		FString actor_full_name_ = Itr->GetFullName();
		FVector actor_location_ = Itr->GetActorLocation();
		FRotator actor_rotation_ = Itr->GetActorRotation();

		ObjectsString += actor_name_ + " " + actor_location_.ToString() + " " + actor_rotation_.ToString()
			+ " MIN:" + Itr->GetComponentsBoundingBox(true).Min.ToString() + " MAX:" + Itr->GetComponentsBoundingBox(true).Max.ToString() +
			((bDebugMode) ? (" " + actor_full_name_ + "\r\n") : "\r\n");
	}
	tick_string_ += ObjectsString + SkeletonsString;
	(new FAutoDeleteAsyncTask<FWriteStringTask>(tick_string_, absolute_file_path))->StartBackgroundTask();
}

void AROXTracker::GenerateSequenceJson()
{
	FString path = scene_save_directory + scene_folder;
	ROXJsonParser::SceneTxtToJson(path, input_scene_TXT_file_name, output_scene_json_file_name);
}

void AROXTracker::ToggleRecording()
{
	bIsRecording = !bIsRecording;
	numFrame = 0;

	if (bRecordMode && bIsRecording && !fileHeaderWritten)
	{
		fileHeaderWritten = true;
		absolute_file_path = scene_save_directory + scene_folder + "/" + scene_file_name_prefix + "_" + GetDateTimeString() + ".txt";
		WriteHeader();
	}
	else if (!bIsRecording)
	{
		fileHeaderWritten = false;
	}
}

FString AROXTracker::GetDateTimeString()
{
	return 	FDateTime::Now().ToString(TEXT("%Y%m%d%-%H%M%S"));
}

/*************************************************************************************/
/*************************************************************************************/
/*************************************************************************************/

APostProcessVolume* AROXTracker::GetPostProcessVolume()
{
	UWorld* World = GetWorld();
	static APostProcessVolume* PostProcessVolume = nullptr;
	static UWorld* CurrentWorld = nullptr; // Check whether the world has been restarted.
	if (PostProcessVolume == nullptr || CurrentWorld != World)
	{
		PostProcessVolume = World->SpawnActor<APostProcessVolume>();
		PostProcessVolume->bUnbound = true;
		CurrentWorld = World;
	}
	return PostProcessVolume;
}

void AROXTracker::SetVisibility(FEngineShowFlags& Target, FEngineShowFlags& Source)
{
	Target.SetStaticMeshes(Source.StaticMeshes);
	Target.SetLandscape(Source.Landscape);
	Target.SetInstancedFoliage(Source.InstancedFoliage);
	Target.SetInstancedGrass(Source.InstancedGrass);
	Target.SetInstancedStaticMeshes(Source.InstancedStaticMeshes);
	Target.SetSkeletalMeshes(Source.SkeletalMeshes);
}

void AROXTracker::VertexColor(FEngineShowFlags& ShowFlags)
{
	FEngineShowFlags PreviousShowFlags(ShowFlags); // Store previous ShowFlags
	ApplyViewMode(VMI_Lit, true, ShowFlags);

	// From MeshPaintEdMode.cpp:2942
	ShowFlags.SetMaterials(false);
	ShowFlags.SetLighting(false);
	ShowFlags.SetBSPTriangles(true);
	ShowFlags.SetVertexColors(true);
	ShowFlags.SetPostProcessing(false);
	ShowFlags.SetHMDDistortion(false);
	ShowFlags.SetTonemapper(false); // This won't take effect here

	GVertexColorViewMode = EVertexColorViewMode::Color;
	SetVisibility(ShowFlags, PreviousShowFlags); // Store the visibility of the scene, such as folliage and landscape.
}

void AROXTracker::PostProcess(FEngineShowFlags& ShowFlags)
{
	FEngineShowFlags PreviousShowFlags(ShowFlags); // Store previous ShowFlags

												   // Basic Settings
	ShowFlags = FEngineShowFlags(EShowFlagInitMode::ESFIM_All0);
	ShowFlags.SetRendering(true);
	ShowFlags.SetStaticMeshes(true);

	ShowFlags.SetMaterials(true);
	// These are minimal setting
	ShowFlags.SetPostProcessing(true);
	ShowFlags.SetPostProcessMaterial(true);
	// ShowFlags.SetVertexColors(true); // This option will change object material to vertex color material, which don't produce surface normal

	GVertexColorViewMode = EVertexColorViewMode::Color;
	SetVisibility(ShowFlags, PreviousShowFlags); // Store the visibility of the scene, such as folliage and landscape.
}

void AROXTracker::Lit()
{
	UWorld* World = GetWorld();
	GetPostProcessVolume()->BlendWeight = 0;

	if (GameShowFlags == nullptr)
	{
		return;
	}
	World->GetGameViewport()->EngineShowFlags = *GameShowFlags;

	CurrentViewmode = EROXViewMode::RVM_Lit;
}

void AROXTracker::Object()
{
	UWorld* World = GetWorld();
	UGameViewportClient* Viewport = World->GetGameViewport();

	PostProcess(Viewport->EngineShowFlags);
	VertexColor(Viewport->EngineShowFlags);

	CurrentViewmode = EROXViewMode::RVM_ObjectMask;
}

void AROXTracker::Depth()
{
	UWorld* World = GetWorld();
	UGameViewportClient* GameViewportClient = World->GetGameViewport();
	FSceneViewport* SceneViewport = GameViewportClient->GetGameViewport();

	PostProcess(GameViewportClient->EngineShowFlags);

	APostProcessVolume* PostProcessVolume = GetPostProcessVolume();
	PostProcessVolume->Settings.WeightedBlendables.Array.Empty();
	PostProcessVolume->Settings.AddBlendable(DepthMat, 1);
	PostProcessVolume->BlendWeight = 1;

	CurrentViewmode = EROXViewMode::RVM_Depth;
}

void AROXTracker::Normal()
{
	UWorld* World = GetWorld();
	UGameViewportClient* GameViewportClient = World->GetGameViewport();
	FSceneViewport* SceneViewport = GameViewportClient->GetGameViewport();

	PostProcess(GameViewportClient->EngineShowFlags);

	APostProcessVolume* PostProcessVolume = GetPostProcessVolume();
	PostProcessVolume->Settings.WeightedBlendables.Array.Empty();
	PostProcessVolume->Settings.AddBlendable(NormalMat, 1);
	PostProcessVolume->BlendWeight = 1;

	CurrentViewmode = EROXViewMode::RVM_Normal;
}

/*******************************************************/

AActor* AROXTracker::CameraNext()
{
	CurrentViewTarget++;
	if (CurrentViewTarget > ViewTargets.Num() - 1)
	{
		CurrentViewTarget = 0;
	}
	return (ViewTargets[CurrentViewTarget]);
}

AActor* AROXTracker::CameraPrev()
{
	CurrentViewTarget--;
	if (CurrentViewTarget < 0)
	{
		CurrentViewTarget = ViewTargets.Num() - 1;
	}
	return (ViewTargets[CurrentViewTarget]);
}

void AROXTracker::TakeScreenshot(EROXViewMode vm)
{
	FString screenshot_filename = screenshots_save_directory + screenshots_folder + "/" + FDateTime::Now().ToString(TEXT("%Y%m%d%-%H%M%S%s"));
	if (vm != EROXViewMode::RVM_Depth)
	{
		HighResSshot(GetWorld()->GetGameViewport(), screenshot_filename, vm);
	}
	else
	{
		TakeDepthScreenshotFolder(screenshot_filename);
	}
}

void AROXTracker::TakeScreenshotFolder(EROXViewMode vm, FString CameraName)
{
	FString screenshot_filename = screenshots_save_directory + screenshots_folder + "/" + json_file_names[CurrentJsonFile] + "/" + ViewmodeString(vm) + "/" + CameraName + "/" + ROXJsonParser::IntToStringDigits(numFrame, 6);
	if (vm != EROXViewMode::RVM_Depth)
	{
		HighResSshot(GetWorld()->GetGameViewport(), screenshot_filename, vm);
	}
	else
	{
		TakeDepthScreenshotFolder(screenshot_filename);
	}
}

void AROXTracker::HighResSshot(UGameViewportClient* ViewportClient, const FString& FullFilename, const EROXViewMode viewmode)
{
	EROXRGBImageFormats rgb_image_format = format_rgb;
	int jpg_quality = 0;
	switch (rgb_image_format)
	{
	case EROXRGBImageFormats::RIF_PNG: jpg_quality = 0;
		break;
	case EROXRGBImageFormats::RIF_JPG95: jpg_quality = 95;
		break;
	case EROXRGBImageFormats::RVM_JPG80: jpg_quality = 80;
		break;
	default: jpg_quality = 0;
		break;
	}

	ViewportClient->Viewport->TakeHighResScreenShot();
	ViewportClient->OnScreenshotCaptured().Clear();
	ViewportClient->OnScreenshotCaptured().AddLambda(
		[FullFilename, viewmode, rgb_image_format, jpg_quality](int32 SizeX, int32 SizeY, const TArray<FColor>& Bitmap)
	{
		TArray<FColor>& RefBitmap = const_cast<TArray<FColor>&>(Bitmap);
		TArray<uint8> RGBData8Bit;
		for (auto& Color : RefBitmap)
		{
			Color.A = 255; // Make sure that all alpha values are opaque.
			RGBData8Bit.Add(Color.R);
			RGBData8Bit.Add(Color.G);
			RGBData8Bit.Add(Color.B);
			RGBData8Bit.Add(Color.A);
		}

		TArray<uint8> ImgData;
		FString FullFilenameExtension;
		if (viewmode == EROXViewMode::RVM_Lit && (rgb_image_format == EROXRGBImageFormats::RVM_JPG80 || rgb_image_format == EROXRGBImageFormats::RIF_JPG95))
		{
			static IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
			static TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG);
			ImageWrapper->SetRaw(RGBData8Bit.GetData(), RGBData8Bit.GetAllocatedSize(), SizeX, SizeY, ERGBFormat::RGBA, 8);
			ImgData = ImageWrapper->GetCompressed(jpg_quality);
			FullFilenameExtension = FullFilename + ".jpg";
		}
		else
		{
			TArray<uint8> CompressedBitmap;
			FImageUtils::CompressImageArray(SizeX, SizeY, RefBitmap, ImgData);
			FullFilenameExtension = FullFilename + ".png";
		}

		(new FAutoDeleteAsyncTask<FScreenshotTask>(ImgData, FullFilenameExtension))->StartBackgroundTask();
	});
}

void AROXTracker::TakeDepthScreenshotFolder(const FString& FullFilename)
{
	//SceneCapture_depth;
	int32 Width = DepthTextureRenderer->SizeX, Height = DepthTextureRenderer->SizeY;
	TArray<FFloat16Color> ImageData;
	FTextureRenderTargetResource* RenderTargetResource;
	ImageData.AddUninitialized(Width * Height);
	RenderTargetResource = DepthTextureRenderer->GameThread_GetRenderTargetResource();
	RenderTargetResource->ReadFloat16Pixels(ImageData);

	if (ImageData.Num() != 0 && ImageData.Num() == Width * Height)
	{
		if (generate_depth)
		{
			TArray<uint16> Grayscaleuint16Data;

			for (auto px : ImageData)
			{
				// Max value float16: 65504.0 -> It is cm, so it can represent up to 655.04m
				// Max value uint16: 65535 (65536 different values) -> It is going to be mm, so it can represent up to 65.535m - 6553.5cm
				float pixelCm = px.R.GetFloat();
				if (pixelCm > 6553.4f || pixelCm < 0.3f)
				{
					Grayscaleuint16Data.Add(0);
				}
				else
				{
					float pixelMm = pixelCm * 10.0f;
					Grayscaleuint16Data.Add((uint16)floorf(pixelMm + 0.5f));
				}
			}

			// Save Png Monochannel 16bits
			static IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
			static TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
			ImageWrapper->SetRaw(Grayscaleuint16Data.GetData(), Grayscaleuint16Data.GetAllocatedSize(), Width, Height, ERGBFormat::Gray, 16);
			const TArray<uint8>& ImgGrayData = ImageWrapper->GetCompressed();
			(new FAutoDeleteAsyncTask<FScreenshotTask>(ImgGrayData, FullFilename + ".png"))->StartBackgroundTask();
		}

		if (generate_depth_txt_cm)
		{
			FString DepthCm("");
			for (auto px : ImageData)
			{
				DepthCm += FString::SanitizeFloat(px.R.GetFloat()) + "\n";
			}
			(new FAutoDeleteAsyncTask<FWriteStringTask>(DepthCm, FullFilename + ".txt"))->StartBackgroundTask();
		}
	}
}

/**********************************************************/
void AROXTracker::ChangeViewmode(EROXViewMode vm)
{
	switch (vm)
	{
	case EROXViewMode::RVM_Lit: Lit();
		break;
	case EROXViewMode::RVM_Depth: Depth();
		break;
	case EROXViewMode::RVM_ObjectMask: Object();
		break;
	case EROXViewMode::RVM_Normal: Normal();
		break;
	}
}

FString AROXTracker::ViewmodeString(EROXViewMode vm)
{
	FString res("rgb");
	switch (vm)
	{
	case EROXViewMode::RVM_Lit: res = "rgb";
		break;
	case EROXViewMode::RVM_Depth: res = "depth";
		break;
	case EROXViewMode::RVM_ObjectMask: res = "mask";
		break;
	case EROXViewMode::RVM_Normal: res = "normal";
		break;
	}
	return res;
}

EROXViewMode AROXTracker::NextViewmode(EROXViewMode vm)
{
	EROXViewMode res(EROXViewMode_First);

	if (vm == EROXViewMode_Last)
	{
		res = EROXViewMode_First;
	}
	else
	{
		bool found = false;
		for (int i = 0; i < (EROXViewModeList.Num() - 1) && !found; i++)
		{
			if (EROXViewModeList[i] == vm)
			{
				res = EROXViewModeList[i + 1];
				found = true;
			}
		}
	}
	return res;
}

void AROXTracker::CacheStaticMeshActors()
{
	// StaticMeshActor dump
	CachedSM.Empty();
	for (TObjectIterator<AStaticMeshActor> Itr; Itr; ++Itr)
	{
		FString fullName = Itr->GetFullName();
		if ((fullName.Contains(Persistence_Level_Filter_Str) || bStandaloneMode || bDebugMode) && Itr->GetStaticMeshComponent()->Mobility == EComponentMobility::Movable)
		{
			CachedSM.Add(*Itr);
		}
	}
}

void AROXTracker::CacheSceneActors(const TArray<FString> &PawnNames, const TArray<FString> &CameraNames)
{
	// StaticMeshActor dump
	CacheStaticMeshActors();

	// Cameras dump
	CameraActors.Empty();
	for (TObjectIterator<ACameraActor> Itr; Itr; ++Itr)
	{
		FString FullName = Itr->GetFullName();
		if (FullName.Contains(Persistence_Level_Filter_Str) || bStandaloneMode)
		{
			for (FString camName : CameraNames)
			{
				if (camName == Itr->GetName())
				{
					CameraActors.Add(*Itr);
				}
			}
		}
	}

	// Pawn dump
	Pawns.Empty();
	for (TObjectIterator<AROXBasePawn> Itr; Itr; ++Itr)
	{
		FString FullName = Itr->GetFullName();
		if (FullName.Contains(Persistence_Level_Filter_Str) || bStandaloneMode)
		{
			for (FString pawnName : PawnNames)
			{
				if (pawnName == Itr->GetActorLabel())
				{
					Pawns.Add(*Itr);
				}
			}
		}
	}
}

void AROXTracker::DisableGravity()
{
	CachedSM_Gravity.Empty();
	CachedSM_Physics.Empty();

	for (AStaticMeshActor* sm : CachedSM)
	{
		CachedSM_Gravity.Add(sm->GetStaticMeshComponent()->IsGravityEnabled());
		CachedSM_Physics.Add(sm->GetStaticMeshComponent()->IsSimulatingPhysics());

		sm->GetStaticMeshComponent()->SetEnableGravity(false);
		sm->GetStaticMeshComponent()->SetSimulatePhysics(false);
	}
}

void AROXTracker::RestoreGravity()
{
	int i = 0;
	for (AStaticMeshActor* sm : CachedSM)
	{
		if (i < CachedSM_Gravity.Num() && i < CachedSM_Physics.Num())
		{
			sm->GetStaticMeshComponent()->SetEnableGravity(CachedSM_Gravity[i]);
			sm->GetStaticMeshComponent()->SetSimulatePhysics(CachedSM_Physics[i]);
		}
		i++;
	}
}

void AROXTracker::ChangeViewmodeDelegate(EROXViewMode vm)
{
	if (vm == EROXViewMode_First)
	{
		for (AROXBasePawn* p : Pawns)
		{
			p->CheckFirstPersonCamera(CameraActors[CurrentCamRebuildMode]);
		}
		ControllerPawn->ChangeViewTarget(CameraActors[CurrentCamRebuildMode]);
		SceneCapture_depth->SetActorLocationAndRotation(CameraActors[CurrentCamRebuildMode]->GetActorLocation(), CameraActors[CurrentCamRebuildMode]->GetActorRotation());
	}
	ChangeViewmode(vm);

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateUObject(this, &AROXTracker::TakeScreenshotDelegate, vm), take_screenshot_delay, false);
}

void AROXTracker::TakeScreenshotDelegate(EROXViewMode vm)
{
	FTimerHandle TimerHandle;
	TakeScreenshotFolder(vm, CameraActors[CurrentCamRebuildMode]->GetActorLabel());

	if (vm == EROXViewMode_Last)
	{
		++CurrentCamRebuildMode;
		if (CurrentCamRebuildMode < CameraActors.Num())
		{
			GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateUObject(this, &AROXTracker::ChangeViewmodeDelegate, EROXViewMode_First), change_viewmode_delay, false);
		}
		else
		{
			CurrentCamRebuildMode = 0;
			GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateUObject(this, &AROXTracker::RebuildModeMain), change_viewmode_delay, false);
		}
	}
	else
	{
		EROXViewMode NextVM = NextViewmode(vm);
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateUObject(this, &AROXTracker::ChangeViewmodeDelegate, NextVM), change_viewmode_delay, false);
	}
}

void AROXTracker::RebuildModeBegin()
{
	if (CurrentJsonFile < start_frames.Num())
	{
		numFrame = start_frames[CurrentJsonFile];
	}
	else
	{
		numFrame = 0;
	}

	JsonReadStartTime = FDateTime::Now().ToUnixTimestamp();
	LastFrameTime = JsonReadStartTime;

	// Print sceneObject JSON 
	FString sceneObject_json_filename = screenshots_save_directory + screenshots_folder + "/" + json_file_names[CurrentJsonFile] + "/sceneObject.json";
	FROXObjectPainter::Get().PrintToJson(sceneObject_json_filename);

	JsonParser = new ROXJsonParser();
	JsonParser->LoadFile(scene_save_directory + scene_folder + "/" + json_file_names[CurrentJsonFile] + ".json");
	CacheSceneActors(JsonParser->GetPawnNames(), JsonParser->GetCameraNames());
	DisableGravity();

	if (JsonParser->GetNumFrames() > 0)
	{
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateUObject(this, &AROXTracker::RebuildModeMain), initial_delay, false);
	}
}

void AROXTracker::RebuildModeMain()
{
	if (numFrame < JsonParser->GetNumFrames())
	{
		int64 currentTime = FDateTime::Now().ToUnixTimestamp();
		PrintStatusToLog(start_frames[CurrentJsonFile], JsonReadStartTime, LastFrameTime, numFrame, currentTime, JsonParser->GetNumFrames());
		LastFrameTime = currentTime;

		currentFrame = JsonParser->GetFrameData(numFrame);

		// Rebuild StaticMesh Actors
		for (AStaticMeshActor* sm : CachedSM)
		{
			FROXActorStateExtended* ObjState = currentFrame.Objects.Find(sm->GetName());
			if (ObjState != nullptr)
			{
				sm->SetActorLocationAndRotation(ObjState->Position, ObjState->Rotation);
			}
		}

		// Rebuild Pawns
		TMap<FName, FTransform> NameTransformMap;
		for (AROXBasePawn* sk : Pawns)
		{
			FString skName = sk->GetActorLabel();
			FROXSkeletonState* SkState = currentFrame.Skeletons.Find(skName);
			if (SkState != nullptr)
			{
				TArray<FString> BoneNames;
				SkState->Bones.GetKeys(BoneNames);

				for (FString BoneName : BoneNames)
				{
					FROXActorState* BoneState = SkState->Bones.Find(BoneName);
					if (BoneState != nullptr)
					{
						FTransform BoneTransform = FTransform(BoneState->Rotation, BoneState->Position);
						sk->EmplaceBoneTransformMap(FName(*BoneName), BoneTransform);
						NameTransformMap = sk->GetNameTransformMap();
					}
				}
			}
		}

		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateUObject(this, &AROXTracker::RebuildModeMain_Camera), place_cameras_delay, false);
	}
	else
	{
		CurrentJsonFile++;
		if (CurrentJsonFile < json_file_names.Num())
		{
			RebuildModeBegin();
		}
		else
		{
			RestoreGravity();
		}
	}
}

void AROXTracker::RebuildModeMain_Camera()
{
	// Rebuild Cameras
	for (ACameraActor* cam : CameraActors)
	{
		FROXActorState* CamState = currentFrame.Cameras.Find(cam->GetName());
		if (CamState != nullptr)
		{
			cam->SetActorLocationAndRotation(CamState->Position, CamState->Rotation);
		}
	}

	++numFrame;
	CurrentCamRebuildMode = 0;
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateUObject(this, &AROXTracker::ChangeViewmodeDelegate, EROXViewMode_First), first_viewmode_of_frame_delay, false);
}

FString SecondsToString(int timeSec)
{
	int seconds = timeSec % 60;
	int timeMin = timeSec / 60;
	int minutes = timeMin % 60;
	int timeHrs = timeMin / 60;

	return FString(FString::FromInt(timeHrs) + "h " + FString::FromInt(minutes) + "min " + FString::FromInt(seconds) + "sec");
}

void AROXTracker::PrintStatusToLog(int startFrame, int64 startTimeSec, int64 lastFrameTimeSec, int currentFrame, int64 currentTimeSec, int totalFrames)
{
	int nDoneFrames = currentFrame - startFrame;

	if (frame_status_output_period > 0 && nDoneFrames != 0 && (nDoneFrames % frame_status_output_period == 0.0f || nDoneFrames == 2 || nDoneFrames == frame_status_output_period / 10 || nDoneFrames == frame_status_output_period / 2))
	{
		int totalFramesForRebuild = totalFrames - startFrame;
		int elapsedTimeSec = (int)(currentTimeSec - startTimeSec);
		int remainingTimeSec = (elapsedTimeSec / nDoneFrames) * (totalFrames - currentFrame);
		int lastFrameElapsedTimeSec = (int)(currentTimeSec - lastFrameTimeSec);

		FString status_msg("Frame " + FString::FromInt(nDoneFrames) + " / " + FString::FromInt(totalFramesForRebuild) + " (" + FString::FromInt(currentFrame) + "/" + FString::FromInt(totalFrames) + ")");
		status_msg += " - Estimated Remaining Time: " + SecondsToString(remainingTimeSec) + " - Last Frame Time: " + FString::FromInt(lastFrameElapsedTimeSec) + "sec - Total Elapsed Time: " + SecondsToString(elapsedTimeSec);

		UE_LOG(LogTemp, Warning, TEXT("%s"), *status_msg);
	}
}
