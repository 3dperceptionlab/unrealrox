// Copyright 2018, 3D Perception Lab

#include "ROXTracker.h"
#include "DateTime.h"
#include "TimerManager.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/PostProcessVolume.h"
#include "ROXObjectPainter.h"
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
	scene_folder("RecordedSequences"),
	screenshots_folder("GeneratedSequences"),
	scene_file_name_prefix("scene"),
	input_scene_TXT_file_name("scene"),
	output_scene_json_file_name("scene"),
	playback_only(false),
	playback_speed_rate(1.0f),
	frame_already_loaded(false),
	generate_rgb(true),
	format_rgb(EROXRGBImageFormats::RIF_JPG95),
	generate_normal(true),
	generate_depth(true),
	generate_object_mask(true),	
	generate_depth_txt_cm(false),
	generated_images_width(1920),
	generated_images_height(1080),
	retrieve_images_from_viewport(false),
	SaveViewmodesInsideCameras(false),
	frame_status_output_period(100),
	fileHeaderWritten(false),
	numFrame(0),
	CurrentViewmode(EROXViewMode_First),
	isMaskedMaterial(false),
	CurrentViewTarget(0),
	CurrentCamRebuildMode(0),
	CurrentJsonFile(0),
	JsonReadStartTime(0),
	LastFrameTime(0)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = bRecordMode;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// Init structures for the bounding box visualizer
	CubeShape = nullptr;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> cubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (cubeFinder.Succeeded())
	{
		CubeShape = (UStaticMesh*)cubeFinder.Object;
	}
	GreenMat = nullptr;
	static ConstructorHelpers::FObjectFinder<UMaterial> matGreen(TEXT("/Game/Common/Green.Green"));
	if (matGreen.Succeeded())
	{
		GreenMat = (UMaterial*)matGreen.Object;
	}
	RedMat = nullptr;
	static ConstructorHelpers::FObjectFinder<UMaterial> matRed(TEXT("/Game/Common/Red.Red"));
	if (matRed.Succeeded())
	{
		RedMat = (UMaterial*)matRed.Object;
	}

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

	MaskMat = nullptr;
	static ConstructorHelpers::FObjectFinder<UMaterial> matMask(TEXT("/Game/Common/ViewModeMats/Mat_InstanceMaskColor.Mat_InstanceMaskColor"));
	if (matMask.Succeeded())
	{
		MaskMat = (UMaterial*)matMask.Object;
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

	//screenshot resolution
	GScreenshotResolutionX = generated_images_width; // 1920  1280
	GScreenshotResolutionY = generated_images_height;  // 1080  720

	for (AROXBasePawn* pawn : Pawns)
	{
		pawn->InitFromTracker(bRecordMode, bDebugMode, this);
		ViewTargets.Add(pawn);

		if (pawn->GetController())
		{
			ControllerPawn = pawn;
		}
	}

	for (ACameraActor* Cam : CameraActors)
	{
		ViewTargets.Add(Cam);
	}

	EROXViewModeList.Empty();
	if (generate_rgb) EROXViewModeList.Add(EROXViewMode::RVM_Lit);
	if (generate_normal) EROXViewModeList.Add(EROXViewMode::RVM_Normal);
	if (generate_depth || generate_depth_txt_cm) EROXViewModeList.Add(EROXViewMode::RVM_Depth);
	if (generate_object_mask) EROXViewModeList.Add(EROXViewMode::RVM_ObjectMask);

	if (EROXViewModeList.Num() == 0) EROXViewModeList.Add(EROXViewMode::RVM_Lit);
	EROXViewMode_First = EROXViewModeList[0];
	EROXViewMode_Last = EROXViewModeList.Last();

	// Check that the last character of directories is a slash
	if (scene_save_directory[scene_save_directory.Len() - 1] != '/')
	{
		scene_save_directory += "/";
	}
	if (screenshots_save_directory[screenshots_save_directory.Len() - 1] != '/')
	{
		screenshots_save_directory += "/";
	}

	// Match list legth of stereo camera baseline with camera list length
	while (CameraActors.Num() > StereoCameraBaselines.Num())
	{
		StereoCameraBaselines.Add(0.0);
	}

	ViewBoundingBoxesInit();
	PrepareMaterials();

	// Playback mode
	if (!bRecordMode)
	{		
		while (json_file_names.Num() > start_frames.Num())
		{
			start_frames.Add(0);
		}

		if (playback_only)
		{
			playback_speed_rate = FMath::Clamp(playback_speed_rate, 0.1f, 3.0f);
		}

		RebuildModeBegin();
	}
}

void AROXTracker::BeginDestroy()
{
	if (isMaskedMaterial)
	{
		ToggleActorMaterials();
		isMaskedMaterial = false;
	}
	Super::BeginDestroy();
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
		ViewBoundingBoxesMain();
	}
}

void AROXTracker::ViewBoundingBoxesInit()
{
	// Aligned BoundingBoxes
	BoundingBoxesVertexes.Empty();
	for (int i = 0; i < ViewAlignedBoundingBox.Num(); ++i)
	{
		BoundingBoxesVertexes.Add(TArray<AStaticMeshActor*>());
		for (int j = 0; j < 8; ++j)
		{
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.Name = FName(*("Vertex" + FString::FromInt(i) + "_" + FString::FromInt(j)));
			AStaticMeshActor* CubeActor = GetWorld()->SpawnActor<AStaticMeshActor>(FVector(0.0f, 0.0f, 0.0f), FRotator(0.0f, 0.0f, 0.0f), ActorSpawnParams);
			CubeActor->GetStaticMeshComponent()->SetStaticMesh(CubeShape);
			CubeActor->GetStaticMeshComponent()->SetMaterial(0, GreenMat);
			CubeActor->SetActorScale3D(FVector(0.02f, 0.02f, 0.02f));
			CubeActor->SetActorEnableCollision(false);
			CubeActor->SetMobility(EComponentMobility::Movable);
			CubeActor->GetStaticMeshComponent()->SetEnableGravity(false);
			CubeActor->GetStaticMeshComponent()->SetSimulatePhysics(false);
			BoundingBoxesVertexes[i].Add(CubeActor);
		}
		BoundingBoxesVertexes[i][0]->SetActorScale3D(FVector(0.03f, 0.03f, 0.03f));
		BoundingBoxesVertexes[i][1]->SetActorScale3D(FVector(0.03f, 0.03f, 0.03f));
	}

	// Oriented BoundingBoxes
	VertexesOBBs.Empty();
	BoundingBoxesVertexesOBB.Empty();
	for (int i = 0; i < ViewOrientedBoundingBox.Num(); ++i)
	{
		FRotator objRot = ViewOrientedBoundingBox[i]->GetActorRotation();
		FVector objLoc = ViewOrientedBoundingBox[i]->GetActorLocation();
		ViewOrientedBoundingBox[i]->SetActorLocation(FVector(0.0f, 0.0f, 0.0f));
		ViewOrientedBoundingBox[i]->SetActorRotation(FRotator(0.0f, 0.0f, 0.0f));
		FBox ABB = ViewOrientedBoundingBox[i]->GetComponentsBoundingBox();
		FVector min = ABB.Min;
		FVector max = ABB.Max;
		FVector diff = max - min;
		VertexesOBBs.Add(TArray<FVector>());
		VertexesOBBs[i].Add(min);
		VertexesOBBs[i].Add(max);
		VertexesOBBs[i].Add(min + FVector(diff.X, 0.0f, 0.0f));
		VertexesOBBs[i].Add(min + FVector(0.0f, diff.Y, 0.0f));
		VertexesOBBs[i].Add(min + FVector(0.0f, 0.0f, diff.Z));
		VertexesOBBs[i].Add(min + FVector(diff.X, diff.Y, 0.0f));
		VertexesOBBs[i].Add(min + FVector(diff.X, 0.0f, diff.Z));
		VertexesOBBs[i].Add(min + FVector(0.0f, diff.Y, diff.Z));

		ViewOrientedBoundingBox[i]->SetActorLocation(objLoc);
		ViewOrientedBoundingBox[i]->SetActorRotation(objRot);

		BoundingBoxesVertexesOBB.Add(TArray<AStaticMeshActor*>());
		for (int j = 0; j < 8; ++j)
		{
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.Name = FName(*("VertexOBB" + FString::FromInt(i) + "_" + FString::FromInt(j)));
			AStaticMeshActor* CubeActor = GetWorld()->SpawnActor<AStaticMeshActor>(FVector(0.0f, 0.0f, 0.0f), FRotator(0.0f, 0.0f, 0.0f), ActorSpawnParams);
			CubeActor->GetStaticMeshComponent()->SetStaticMesh(CubeShape);
			CubeActor->GetStaticMeshComponent()->SetMaterial(0, RedMat);
			CubeActor->SetActorScale3D(FVector(0.02f, 0.02f, 0.02f));
			CubeActor->SetActorEnableCollision(false);
			CubeActor->SetMobility(EComponentMobility::Movable);
			CubeActor->GetStaticMeshComponent()->SetEnableGravity(false);
			CubeActor->GetStaticMeshComponent()->SetSimulatePhysics(false);
			BoundingBoxesVertexesOBB[i].Add(CubeActor);
		}
		BoundingBoxesVertexesOBB[i][0]->SetActorScale3D(FVector(0.03f, 0.03f, 0.03f));
		BoundingBoxesVertexesOBB[i][1]->SetActorScale3D(FVector(0.03f, 0.03f, 0.03f));
	}
}

void AROXTracker::ViewBoundingBoxesMain()
{
	// Aligned BoundingBoxes
	for (int i = 0; i < ViewAlignedBoundingBox.Num(); ++i)
	{
		TArray<FVector> Vertexes;

		FBox ABB = ViewAlignedBoundingBox[i]->GetComponentsBoundingBox();
		FVector min = ABB.Min;
		FVector max = ABB.Max;
		FVector diff = max - min;
		Vertexes.Add(min);
		Vertexes.Add(max);
		Vertexes.Add(min + FVector(diff.X, 0.0f, 0.0f));
		Vertexes.Add(min + FVector(0.0f, diff.Y, 0.0f));
		Vertexes.Add(min + FVector(0.0f, 0.0f, diff.Z));
		Vertexes.Add(min + FVector(diff.X, diff.Y, 0.0f));
		Vertexes.Add(min + FVector(diff.X, 0.0f, diff.Z));
		Vertexes.Add(min + FVector(0.0f, diff.Y, diff.Z));

		for (int j = 0; j < Vertexes.Num(); ++j)
		{
			BoundingBoxesVertexes[i][j]->SetActorLocation(Vertexes[j]);
		}
	}

	// Oriented BoundingBoxes
	for (int i = 0; i < ViewOrientedBoundingBox.Num(); ++i)
	{
		FVector objLoc = ViewOrientedBoundingBox[i]->GetActorLocation();
		FRotator objRot = ViewOrientedBoundingBox[i]->GetActorRotation();

		for (int j = 0; j < BoundingBoxesVertexesOBB[i].Num(); ++j)
		{
			BoundingBoxesVertexesOBB[i][j]->SetActorLocation(objRot.RotateVector(VertexesOBBs[i][j]) + objLoc);
		}
	}
}


//    ____                        _ _             
//   |  _ \ ___  ___ ___  _ __ __| (_)_ __   __ _ 
//   | |_) / _ \/ __/ _ \| '__/ _` | | '_ \ / _` |
//   |  _ <  __/ (_| (_) | | | (_| | | | | | (_| |
//   |_| \_\___|\___\___/|_|  \__,_|_|_| |_|\__, |
//                                          |___/ 

FString GetOBBPointsStr(AStaticMeshActor* sm)
{
	FString OBBStr("");

	auto mob = sm->GetStaticMeshComponent()->Mobility;
	bool mobility_changed = false;

	if (mob != EComponentMobility::Movable)
	{
		sm->SetMobility(EComponentMobility::Movable);
		mobility_changed = true;
	}
	FVector sm_location = sm->GetActorLocation();
	FRotator sm_rotation = sm->GetActorRotation();
	sm->SetActorLocation(FVector(0.0f, 0.0f, 0.0f));
	sm->SetActorRotation(FRotator(0.0f, 0.0f, 0.0f));
	FBox sm_bbox = sm->GetComponentsBoundingBox();
	sm->SetActorLocation(sm_location);
	sm->SetActorRotation(sm_rotation);
	if (mobility_changed)
	{
		sm->SetMobility(mob);
	}

	FVector min_max_diff = sm_bbox.Max - sm_bbox.Min;

	OBBStr += "OBB:" + (sm_bbox.Min + FVector(min_max_diff.X, min_max_diff.Y, 0.0f)).ToString() + " "
		+ (sm_bbox.Max).ToString() + " "
		+ (sm_bbox.Min + FVector(0.0f, min_max_diff.Y, min_max_diff.Z)).ToString() + " "
		+ (sm_bbox.Min + FVector(0.0f, min_max_diff.Y, 0.0f)).ToString() + " "
		+ (sm_bbox.Min + FVector(min_max_diff.X, 0.0f, 0.0f)).ToString() + " "
		+ (sm_bbox.Min + FVector(min_max_diff.X, 0.0f, min_max_diff.Z)).ToString() + " "
		+ (sm_bbox.Min + FVector(0.0f, 0.0f, min_max_diff.Z)).ToString() + " "
		+ (sm_bbox.Min).ToString();

	return OBBStr;
}

void AROXTracker::WriteHeader()
{
	//Camera Info
	FString begin_string_ = "Cameras " + FString::FromInt(CameraActors.Num()) + "\r\n";
	for (int i = 0; i < CameraActors.Num(); i++)
	{
		ACameraActor* CameraActor = CameraActors[i];
		float stereo_dist(StereoCameraBaselines[i]);
		float field_of_view(CameraActor->GetCameraComponent()->FieldOfView);
		begin_string_ += CameraActor->GetName() + " " + FString::SanitizeFloat(stereo_dist) + " " + FString::SanitizeFloat(field_of_view) + "\r\n";
	}

	// Movable StaticMeshActor dump
	CacheStaticMeshActors();
	FString staticdump("");
	for (AStaticMeshActor* sm : CachedSM)
	{
		staticdump += sm->GetName() + " " + GetOBBPointsStr(sm) + "\r\n";
	}
	begin_string_ += "Objects " + FString::FromInt(CachedSM.Num()) + "\r\n" + staticdump;

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
		if ((fullName.Contains(Persistence_Level_Filter_Str) || bStandaloneMode || bDebugMode) && Itr->GetStaticMeshComponent()->Mobility != EComponentMobility::Movable)
		{
			n_nonmovable++;
			FString actor_name_ = Itr->GetName();
			FString actor_full_name_ = Itr->GetFullName();
			FVector actor_location_ = Itr->GetActorLocation();
			FRotator actor_rotation_ = Itr->GetActorRotation();
			FBox actor_bbox = Itr->GetComponentsBoundingBox(true);
			nonmovabledump += actor_name_ + " " + actor_location_.ToString() + " " + actor_rotation_.ToString();
			nonmovabledump += " MIN:" + actor_bbox.Min.ToString() + " MAX:" + actor_bbox.Max.ToString();
			nonmovabledump += " " + GetOBBPointsStr(*Itr);
			nonmovabledump += ((bDebugMode) ? (" " + actor_full_name_ + "\r\n") : "\r\n");
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

	TMap<AActor*, EROXMeshState> interaction_data = ControllerPawn->GetInteractionData();

	// StaticMeshActor dump
	for (auto Itr : CachedSM)
	{
		FString actor_name_ = Itr->GetName();
		FString actor_full_name_ = Itr->GetFullName();
		FVector actor_location_ = Itr->GetActorLocation();
		FRotator actor_rotation_ = Itr->GetActorRotation();

		FString state_str = "None";
		EROXMeshState* state = interaction_data.Find(Itr);
		if (state != nullptr)
		{
			state_str = MeshStateToString(*state);
		}

		ObjectsString += actor_name_ + " " + actor_location_.ToString() + " " + actor_rotation_.ToString();
		ObjectsString += " MIN:" + Itr->GetComponentsBoundingBox(true).Min.ToString() + " MAX:" + Itr->GetComponentsBoundingBox(true).Max.ToString();
		ObjectsString += " " + state_str + " ";
		ObjectsString += ((bDebugMode) ? (" " + actor_full_name_ + "\r\n") : "\r\n");
	}
	tick_string_ += ObjectsString + SkeletonsString;
	(new FAutoDeleteAsyncTask<FWriteStringTask>(tick_string_, absolute_file_path))->StartBackgroundTask();
}

FString AROXTracker::MeshStateToString(EROXMeshState state)
{
	FString str("None");
	switch (state)
	{
	case EROXMeshState::RMS_Grasped_L:
		str = "GraspedL";
		break;
	case EROXMeshState::RMS_Grasped_R:
		str = "GraspedR";
		break;
	case EROXMeshState::RMS_Interacted:
		str = "Interacted";
		break;
	case EROXMeshState::RMS_Body:
		str = "Body";
		break;
	case EROXMeshState::RMS_None:
		str = "None";
		break;
	default:
		str = "None";
		break;
	}

	return str;
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


//   __     ___                                   _           
//   \ \   / (_) _____      ___ __ ___   ___   __| | ___  ___ 
//    \ \ / /| |/ _ \ \ /\ / / '_ ` _ \ / _ \ / _` |/ _ \/ __|
//     \ V / | |  __/\ V  V /| | | | | | (_) | (_| |  __/\__ \
//      \_/  |_|\___| \_/\_/ |_| |_| |_|\___/ \__,_|\___||___/
//                                                            

/*
*/
FColor AROXTracker::AssignColor(int idx_in)
{
	/* RGB values are assigned like this:
	00 ->	255	0	0	| 10 -> 127	255	255	| 21 ->	255	127	127
	01 ->	0	255	0	| 11 -> 0	127	0	| 21 ->	127	0	127
	02 ->	0	0	255	| 12 -> 0	127	255	| 22 ->	127	255	127
	03 ->	0	255	255	| 13 -> 255	127	0	| 23 ->	127	127	0
	04 ->	255	0	255	| 14 ->	255	127	255	| 24 ->	127	127	255
	05 ->	255	255	0	| 15 ->	0	0	127	| 25 ->	127	127	127
	06 ->	255	255	255	| 16 ->	0	255	127	| 26 ->	63	0	0
	07 ->	127	0	0	| 17 ->	255	0	127	| 27 ->	63	0	255
	08 ->	127	0	255	| 18 ->	255	255	127	| 28 ->	63	0	127
	09 ->	127	255	0	| 19 ->	0	127	127	| 29 ->	63	255	0
	*/
	const static uint8 ChannelValues[] = { 0, 255, 127, 63, 191, 31, 95, 159, 223, 15, 47, 79, 111, 143, 175, 207, 239, 7, 23, 39, 55, 71, 87, 103, 119, 135, 151, 167, 183, 199, 215, 231 };

	FColor color = FColor(0, 0, 0, 255);
	int idx = idx_in + 1; // Avoid black to be assigned

	if (idx > 0 && idx < 32760) // Max colors that can be assigned combining the previous channel values
	{
		// VAL variable represents two things (related between them):
		// - The position index on ChannelValues array of the last channel value.
		// - The number of previous channel values
		int val = FMath::FloorToInt(FMath::Pow(idx, (float)(1.0f / 3.0f)));
		int prev_combinations = FMath::Pow(val, 3);

		int combination_idx = idx - prev_combinations;
		int sqr_val = val * val;
		int n_comb_double = 3 * sqr_val;
		int n_comb_unit = 3 * val;

		uint8 color_arr[3] = {0, 0, 0};
		if (combination_idx >= 0 && combination_idx < n_comb_double)
		{
			int partial_group_idx = combination_idx / sqr_val;
			int partial_line_idx = combination_idx % sqr_val;			

			int inner_group_idx = partial_line_idx / val;
			int inner_line_idx = partial_line_idx % val;
			int low = 0, high = 2;
			if (partial_group_idx == 0) low = 1;
			else if (partial_group_idx == 2) high = 1;

			color_arr[partial_group_idx] = ChannelValues[val];
			color_arr[low] = ChannelValues[inner_group_idx];
			color_arr[high] = ChannelValues[inner_line_idx];
		}
		else if (combination_idx >= n_comb_double && combination_idx < (n_comb_unit + n_comb_double))
		{
			for (int i = 0; i < 3; ++i) color_arr[i] = ChannelValues[val];
			int partial_comb_idx = combination_idx - n_comb_double;
			int partial_group_idx = partial_comb_idx / val;
			int partial_line_idx = partial_comb_idx % val;

			color_arr[partial_group_idx] = ChannelValues[partial_line_idx];
		}
		else if (combination_idx == (n_comb_unit + n_comb_double))
		{
			for (int i = 0; i < 3; ++i) color_arr[i] = ChannelValues[val];
		}

		color.R = color_arr[0];
		color.G = color_arr[1];
		color.B = color_arr[2];
		color.A = 255;
	}
	return color;
}

int AROXTracker::GetIdxFromColor(FColor color)
{
	int idx = 0;
	const static uint8 ChannelValues[] = { 0, 255, 127, 63, 191, 31, 95, 159, 223, 15, 47, 79, 111, 143, 175, 207, 239, 7, 23, 39, 55, 71, 87, 103, 119, 135, 151, 167, 183, 199, 215, 231 };
	const static uint8 ChannelValuesSize = 32;
	uint8 color_input[3] = { color.R, color.G, color.B };
	uint8 color_arr[3] = { 0, 0, 0 };

	// Translation from colors to positions in ChannelValues array.
	// Max of the three position values is stored.
	// If max value is repeated, it is taken into account.
	bool found = true;
	int max = -1;
	int max_pos = -1, max_pos2 = -1, not_max_pos = -1, not_max_pos2 = -1;
	bool isMaxRepeatedTwice = false, isAllEqual = false;
	for (int i = 0; i < 3 && found; ++i)
	{
		found = false;
		for (int j = 0; j < ChannelValuesSize && !found; ++j)
		{
			if (color_input[i] == ChannelValues[j])
			{
				found = true;
				color_arr[i] = j;
				if (max == j)
				{
					if (!isMaxRepeatedTwice)
					{
						isMaxRepeatedTwice = true;
						max_pos2 = i;

						if (i == 1)	not_max_pos = 2;
						else if (i == 2)
						{
							if(max_pos == 0) not_max_pos = 1;
							else if(max_pos == 1) not_max_pos = 0;
						}
					}
					else
					{
						isMaxRepeatedTwice = false;
						isAllEqual = true;
					}
				}
				else if (max < j)
				{
					max = j;
					isMaxRepeatedTwice = false;
					max_pos = i;
					max_pos2 = -1;
				}
			}
		}
	}

	if (found)
	{
		int prev_combinations = FMath::Pow(max, 3);

		// Case 1 -> MAX - MAX - MAX (Ex: 127 127 127)
		if (isAllEqual)
		{
			idx = FMath::Pow(max + 1, 3) - 1;
		}
		// Case 2 -> MAX - MAX - X (Ex: 127 127 255)
		else if (isMaxRepeatedTwice)
		{
			int case3_combinations = FMath::Pow(max, 2) * 3;
			idx = prev_combinations + case3_combinations + max * not_max_pos + color_arr[not_max_pos];
		}
		// Case 3 -> MAX - X - X (Ex: 127 0 255)
		else
		{
			if (max_pos == 0) { not_max_pos = 1; not_max_pos2 = 2; }
			else if (max_pos == 1) { not_max_pos = 0; not_max_pos2 = 2; }
			else { not_max_pos = 0; not_max_pos2 = 1; }

			int case3_prev_combinations = FMath::Pow(max, 2) * max_pos;
			idx = prev_combinations + case3_prev_combinations + color_arr[not_max_pos] * max + color_arr[not_max_pos2];
		}
	}

	return idx - 1;
}

void AROXTracker::PrepareMaterials()
{
	bool file_loaded = false;
	TMap<FName, FROXSceneObject> SceneObjects;
	if (json_file_names.Num() > 0)
	{
		FString sceneObject_json_filename = screenshots_save_directory + screenshots_folder + "/" + json_file_names[CurrentJsonFile] + "/sceneObject.json";
		SceneObjects = ROXJsonParser::LoadSceneObjects(sceneObject_json_filename);
		file_loaded = SceneObjects.Num() > 0;
	}

	TArray<int> used_idxs;
	if (file_loaded)
	{
		TArray<FName> SceneObjects_name;
		SceneObjects.GetKeys(SceneObjects_name);
		for (FName SceneObject_name : SceneObjects_name)
		{
			FROXSceneObject* SceneObject_data = SceneObjects.Find(SceneObject_name);
			int color_idx = GetIdxFromColor(SceneObject_data->instance_color);
			if (color_idx > 0)
			{
				used_idxs.Add(color_idx);
			}
		}
		used_idxs.Sort();
	}

	int comp_idx = 0;
	int used_idxs_i = 0;
	for (TObjectIterator<AActor> Itr; Itr; ++Itr)
	{
		TArray<UMeshComponent*> Components;
		(*Itr)->GetComponents<UMeshComponent>(Components);
		FString ActorFullName = (*Itr)->GetFullName();

		if (Components.Num() > 0 && ActorFullName.Contains(Persistence_Level_Filter_Str) && !(*Itr)->IsA(ACameraActor::StaticClass()) && !(*Itr)->IsA(ASceneCapture2D::StaticClass()))
		{
			TArray<FROXMeshComponentMaterials> ComponentMaterials;
			FColor ActorColor = FColor::Black;
			if (file_loaded)
			{
				FROXSceneObject* SceneObject = SceneObjects.Find(FName(*(*Itr)->GetName()));
				if (SceneObject != nullptr)
				{
					ActorColor = SceneObject->instance_color;
				}
				else
				{
					while (comp_idx == used_idxs[used_idxs_i])
					{
						comp_idx++;
						if (used_idxs_i < (used_idxs.Num() - 1)) used_idxs_i++;
					}
					ActorColor = AssignColor(comp_idx);
					++comp_idx;
				}
			}
			else
			{
				ActorColor = AssignColor(comp_idx);
				++comp_idx;
			}

			for (UMeshComponent* MeshComponent : Components)
			{
				FROXMeshComponentMaterials MaterialStruct;
				MaterialStruct.MeshComponent = MeshComponent;
				MaterialStruct.MaskMaterialColor = ActorColor;
				//MaterialStruct.MaskMaterialLinearColor = FLinearColor::FromPow22Color(MaterialStruct.MaskMaterialColor);
				MaterialStruct.MaskMaterialLinearColor = FLinearColor::FromSRGBColor(MaterialStruct.MaskMaterialColor);
				MaterialStruct.MaskMaterial = UMaterialInstanceDynamic::Create(MaskMat, this);
				MaterialStruct.MaskMaterial->SetVectorParameterValue("MatColor", MaterialStruct.MaskMaterialLinearColor);
				
				//UE_LOG(LogUnrealROX, Warning, TEXT("|-%s"), *(MeshComponent->GetName()));
				/*FString msg = (*Itr)->GetName() + " " + FString::FromInt(comp_idx) + " " + FString::FromInt(GetIdxFromColor(MaterialStruct.MaskMaterialColor));
				msg += ": R: " + FString::FromInt(MaterialStruct.MaskMaterialColor.R) + " G: " + FString::FromInt(MaterialStruct.MaskMaterialColor.G) + " B: " + FString::FromInt(MaterialStruct.MaskMaterialColor.B);
				UE_LOG(LogUnrealROX, Warning, TEXT("%s"), *msg);*/

				TArray <UMaterialInterface*> Materials = MeshComponent->GetMaterials();
				for (UMaterialInterface* Material : Materials)
				{
					if (Material != NULL)
					{
						MaterialStruct.DefaultMaterials.Add(Material);
						//UE_LOG(LogUnrealROX, Warning, TEXT("__|-%s"), *(Material->GetName()));
					}
				}

				ComponentMaterials.Add(MaterialStruct);
			}

			MeshMaterials.Add(*Itr, ComponentMaterials);
		}
	}

	if (json_file_names.Num() < 1)
	{
		FString sceneObject_json_filename = screenshots_save_directory + screenshots_folder + "/sceneObject.json";
		ROXJsonParser::WriteSceneObjects(MeshMaterials, sceneObject_json_filename);
	}
}

void AROXTracker::ToggleActorMaterials()
{
	isMaskedMaterial = !isMaskedMaterial;
	for (const TPair<AActor*, TArray<FROXMeshComponentMaterials>>& pair : MeshMaterials)
	{
		AActor* actor = pair.Key;
		const TArray<FROXMeshComponentMaterials> components = pair.Value;

		for (FROXMeshComponentMaterials component : components)
		{
			for (int i = 0; i < component.DefaultMaterials.Num(); ++i)
			{
				if (isMaskedMaterial)
				{
					component.MeshComponent->SetMaterial(i, component.MaskMaterial);
				}
				else
				{
					component.MeshComponent->SetMaterial(i, component.DefaultMaterials[i]);
				}
			}
		}
	}
}

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
	ShowFlags.SetPostProcessMaterial(false);
	ShowFlags.SetHMDDistortion(false);
	ShowFlags.SetTonemapper(false); // This won't take effect here
	ShowFlags.DisableAdvancedFeatures();

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

ASceneCapture2D* AROXTracker::SpawnSceneCapture(FString ActorName)
{
	FActorSpawnParameters ActorSpawnParams;
	ActorSpawnParams.Name = FName(*ActorName);
	ASceneCapture2D* SceneCapture = GetWorld()->SpawnActor<ASceneCapture2D>(ASceneCapture2D::StaticClass(), ActorSpawnParams);
	SceneCapture->GetCaptureComponent2D()->TextureTarget = NewObject<UTextureRenderTarget2D>();
	SceneCapture->GetCaptureComponent2D()->TextureTarget->InitAutoFormat(generated_images_width, generated_images_height);
	SceneCapture->GetCaptureComponent2D()->PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
	SceneCapture->GetCaptureComponent2D()->PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
	SceneCapture->GetCaptureComponent2D()->PostProcessSettings.AutoExposureMinBrightness = 1.0f;
	SceneCapture->GetCaptureComponent2D()->PostProcessSettings.AutoExposureMaxBrightness = 1.0f;
	return SceneCapture;
}

void AROXTracker::SetViewmodeSceneCapture(ASceneCapture2D* SceneCapture, EROXViewMode vm)
{
	switch (vm)
	{
	case EROXViewMode::RVM_Lit:
		SceneCapture->GetCaptureComponent2D()->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		SceneCapture->GetCaptureComponent2D()->TextureTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
		SceneCapture->GetCaptureComponent2D()->TextureTarget->TargetGamma = 2.2;
		SceneCapture->GetCaptureComponent2D()->PostProcessBlendWeight = 0;
		break;
	case EROXViewMode::RVM_Depth:
		SceneCapture->GetCaptureComponent2D()->CaptureSource = ESceneCaptureSource::SCS_SceneDepth;
		SceneCapture->GetCaptureComponent2D()->TextureTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA16f;
		SceneCapture->GetCaptureComponent2D()->TextureTarget->TargetGamma = 0;
		break;
	case EROXViewMode::RVM_ObjectMask:
		SceneCapture->GetCaptureComponent2D()->CaptureSource = ESceneCaptureSource::SCS_BaseColor;
		SceneCapture->GetCaptureComponent2D()->TextureTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
		SceneCapture->GetCaptureComponent2D()->TextureTarget->TargetGamma = 1;
		break;
	case EROXViewMode::RVM_Normal:
		SceneCapture->GetCaptureComponent2D()->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		SceneCapture->GetCaptureComponent2D()->TextureTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
		PostProcess(SceneCapture->GetCaptureComponent2D()->ShowFlags);
		check(NormalMat);
		SceneCapture->GetCaptureComponent2D()->PostProcessSettings.WeightedBlendables.Array.Empty();
		SceneCapture->GetCaptureComponent2D()->PostProcessSettings.AddBlendable(NormalMat, 1);
		SceneCapture->GetCaptureComponent2D()->PostProcessBlendWeight = 1;
		break;
	}
	
	//SceneCapture->GetCaptureComponent2D()->FOVAngle = ;
}

ASceneCapture2D* AROXTracker::GetSceneCapture(EROXViewMode vm)
{
	ASceneCapture2D* SceneCapture(DefaultSceneCapture);

	if (generate_rgb || generate_depth || generate_object_mask || generate_normal)
	{
		switch (vm)
		{
		case EROXViewMode::RVM_Lit:
			SceneCapture = SceneCapture_Lit[CurrentCamRebuildMode];
			break;
		case EROXViewMode::RVM_Depth:
			SceneCapture = SceneCapture_Depth[CurrentCamRebuildMode];
			break;
		case EROXViewMode::RVM_ObjectMask:
			SceneCapture = SceneCapture_Mask[CurrentCamRebuildMode];
			break;
		case EROXViewMode::RVM_Normal:
			SceneCapture = SceneCapture_Normal[CurrentCamRebuildMode];
			break;
		}
	}

	return SceneCapture;
}

void AROXTracker::Lit()
{
	UWorld* World = GetWorld();
	GetPostProcessVolume()->BlendWeight = 0;
	if (GameShowFlags == nullptr) return;
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


//    ____  _             _                _    
//   |  _ \| | __ _ _   _| |__   __ _  ___| | __
//   | |_) | |/ _` | | | | '_ \ / _` |/ __| |/ /
//   |  __/| | (_| | |_| | |_) | (_| | (__|   < 
//   |_|   |_|\__,_|\__, |_.__/ \__,_|\___|_|\_\
//                  |___/                       

void AROXTracker::TakeScreenshot(EROXViewMode vm)
{
	FString screenshot_filename = screenshots_save_directory + screenshots_folder + "/" + FDateTime::Now().ToString(TEXT("%Y%m%d%-%H%M%S%s"));
	if (vm != EROXViewMode::RVM_Depth)
	{
		HighResSshot(GetWorld()->GetGameViewport(), screenshot_filename, vm);
	}
	else
	{
		TakeDepthScreenshotFolder(GetSceneCapture(vm), screenshot_filename);
	}
}

void AROXTracker::TakeScreenshotFolder(EROXViewMode vm, FString CameraName)
{
	FString screenshot_filename = screenshots_save_directory + screenshots_folder + "/" + json_file_names[CurrentJsonFile] + "/";
	if (!SaveViewmodesInsideCameras)
	{
		screenshot_filename += ViewmodeString(vm) + "/" + CameraName + "/" + ROXJsonParser::IntToStringDigits(numFrame, 6);
	}
	else
	{
		screenshot_filename += CameraName + "/" + ViewmodeString(vm) + "/" + ROXJsonParser::IntToStringDigits(numFrame, 6);
	}
	if (vm == EROXViewMode::RVM_Depth)
	{
		TakeDepthScreenshotFolder(GetSceneCapture(vm), screenshot_filename);
	}
	else
	{
		if (retrieve_images_from_viewport)
		{
			HighResSshot(GetWorld()->GetGameViewport(), screenshot_filename, vm);
		}
		else
		{
			TakeRenderTargetScreenshotFolder(GetSceneCapture(vm), screenshot_filename, vm);
		}
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
			RGBData8Bit.Add(255);
		}

		TArray<uint8> ImgData;
		FString FullFilenameExtension;
		if (viewmode == EROXViewMode::RVM_Lit && (rgb_image_format == EROXRGBImageFormats::RVM_JPG80 || rgb_image_format == EROXRGBImageFormats::RIF_JPG95))
		{
			static IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
			static TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG);
			//ImageWrapper->SetRaw(RGBData8Bit.GetData(), RGBData8Bit.GetAllocatedSize(), SizeX, SizeY, ERGBFormat::RGBA, 8);
			ImageWrapper->SetRaw(RefBitmap.GetData(), RefBitmap.GetAllocatedSize(), SizeX, SizeY, ERGBFormat::BGRA, 8);
			ImgData = ImageWrapper->GetCompressed(jpg_quality);
			FullFilenameExtension = FullFilename + ".jpg";
		}
		else
		{
			FImageUtils::CompressImageArray(SizeX, SizeY, RefBitmap, ImgData);
			FullFilenameExtension = FullFilename + ".png";
		}

		(new FAutoDeleteAsyncTask<FScreenshotTask>(ImgData, FullFilenameExtension))->StartBackgroundTask();
	});
}

void AROXTracker::TakeDepthScreenshotFolder(ASceneCapture2D* SceneCapture, const FString& FullFilename)
{
	int32 Width = SceneCapture->GetCaptureComponent2D()->TextureTarget->SizeX;
	int32 Height = SceneCapture->GetCaptureComponent2D()->TextureTarget->SizeY;
	TArray<FFloat16Color> ImageData;
	FTextureRenderTargetResource* RenderTargetResource;
	ImageData.AddUninitialized(Width * Height);
	RenderTargetResource = SceneCapture->GetCaptureComponent2D()->TextureTarget->GameThread_GetRenderTargetResource();
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

void AROXTracker::TakeRenderTargetScreenshotFolder(ASceneCapture2D* SceneCapture, const FString& FullFilename, const EROXViewMode viewmode)
{
	int32 Width = SceneCapture->GetCaptureComponent2D()->TextureTarget->SizeX;
	int32 Height = SceneCapture->GetCaptureComponent2D()->TextureTarget->SizeY;	
	FTextureRenderTargetResource* RenderTargetResource;
	RenderTargetResource = SceneCapture->GetCaptureComponent2D()->TextureTarget->GameThread_GetRenderTargetResource();

	TArray<FLinearColor> ImageDataLC;
	ImageDataLC.AddUninitialized(Width * Height);
	RenderTargetResource->ReadLinearColorPixels(ImageDataLC);

	TArray<FColor> ImageData;
	ImageData.AddUninitialized(Width * Height);

	if (ImageData.Num() != 0 && ImageData.Num() == Width * Height)
	{
		for (int i = 0; i < ImageData.Num(); ++i)
		{
			if (viewmode == EROXViewMode::RVM_ObjectMask)
			{
				ImageData[i] = ImageDataLC[i].ToFColor(true);
			}
			else
			{
				ImageData[i] = ImageDataLC[i].ToFColor(false);
			}
			ImageData[i].A = 255;
		}

		TArray<uint8> ImgData;
		FString FullFilenameExtension;

		if (viewmode == EROXViewMode::RVM_Lit && generate_rgb && (format_rgb == EROXRGBImageFormats::RVM_JPG80 || format_rgb == EROXRGBImageFormats::RIF_JPG95))
		{
			static IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
			static TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG);
			ImageWrapper->SetRaw(ImageData.GetData(), ImageData.GetAllocatedSize(), Width, Height, ERGBFormat::BGRA, 8);
			ImgData = ImageWrapper->GetCompressed(format_rgb == EROXRGBImageFormats::RIF_JPG95 ? 95 : 80);
			FullFilenameExtension = FullFilename + ".jpg";
		}
		else if (  (viewmode == EROXViewMode::RVM_Lit && generate_rgb)
				|| (viewmode == EROXViewMode::RVM_ObjectMask && generate_object_mask)
				|| (viewmode == EROXViewMode::RVM_Normal && generate_normal))
		{
			FImageUtils::CompressImageArray(Width, Height, ImageData, ImgData);
			FullFilenameExtension = FullFilename + ".png";
		}

		(new FAutoDeleteAsyncTask<FScreenshotTask>(ImgData, FullFilenameExtension))->StartBackgroundTask();
	}
}

/**********************************************************/

void AROXTracker::SpawnCamerasPlayback(FROXCameraConfig camConfig, int stereo)
{
	FString stereo_str("");
	if (stereo > 0) stereo_str = "_Right";
	else if (stereo < 0) stereo_str = "_Left";

	if (stereo == 0)	StereoCameraBaselines.Add(camConfig.StereoBaseline);
	else				StereoCameraBaselines.Add(-1);

	FActorSpawnParameters ActorSpawnParams;
	ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ActorSpawnParams.bDeferConstruction = true;
	ActorSpawnParams.Name = FName(*(camConfig.CameraName + stereo_str + "_PLAYBACK"));
	ACameraActor* cam_spawn = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), ActorSpawnParams);
	cam_spawn->SetActorLabel(camConfig.CameraName + stereo_str);
	cam_spawn->GetCameraComponent()->SetFieldOfView(camConfig.FieldOfView);
	cam_spawn->GetCameraComponent()->bConstrainAspectRatio = false;
	cam_spawn->GetCameraComponent()->PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
	cam_spawn->GetCameraComponent()->PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
	cam_spawn->GetCameraComponent()->PostProcessSettings.AutoExposureMinBrightness = 1.0f;
	cam_spawn->GetCameraComponent()->PostProcessSettings.AutoExposureMaxBrightness = 1.0f;
	CameraActors.Add(cam_spawn);

	if (generate_rgb)
	{
		ASceneCapture2D* SceneCapture_Lit_Aux = SpawnSceneCapture("SceneCaptureLit_" + camConfig.CameraName + stereo_str);
		SetViewmodeSceneCapture(SceneCapture_Lit_Aux, EROXViewMode::RVM_Lit);
		SceneCapture_Lit.Add(SceneCapture_Lit_Aux);
	}
	
	if (generate_depth)
	{
		ASceneCapture2D* SceneCapture_Depth_Aux = SpawnSceneCapture("SceneCaptureDepth_" + camConfig.CameraName + stereo_str);
		SetViewmodeSceneCapture(SceneCapture_Depth_Aux, EROXViewMode::RVM_Depth);
		SceneCapture_Depth.Add(SceneCapture_Depth_Aux);
	}
	
	if (generate_normal)
	{
		ASceneCapture2D* SceneCapture_Normal_Aux = SpawnSceneCapture("SceneCaptureNormal_" + camConfig.CameraName + stereo_str);
		SetViewmodeSceneCapture(SceneCapture_Normal_Aux, EROXViewMode::RVM_Normal);
		SceneCapture_Normal.Add(SceneCapture_Normal_Aux);
	}

	if (generate_object_mask)
	{
		ASceneCapture2D* SceneCapture_Mask_Aux = SpawnSceneCapture("SceneCaptureMask_" + camConfig.CameraName + stereo_str);
		SetViewmodeSceneCapture(SceneCapture_Mask_Aux, EROXViewMode::RVM_ObjectMask);
		SceneCapture_Mask.Add(SceneCapture_Mask_Aux);
	}
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

void AROXTracker::CacheSceneActors(const TArray<FROXPawnInfo> &PawnsInfo, const TArray<FROXCameraConfig> &CameraConfigs)
{
	// StaticMeshActor dump
	CacheStaticMeshActors();

	// Cameras dump
	CameraActors.Empty();
	StereoCameraBaselines.Empty();
	SceneCapture_Depth.Empty();
	SceneCapture_Lit.Empty();
	SceneCapture_Normal.Empty();
	SceneCapture_Mask.Empty();

	DefaultSceneCapture = SpawnSceneCapture("DefaultSceneCapture");
	SetViewmodeSceneCapture(DefaultSceneCapture, EROXViewMode::RVM_Lit);

	/*for (TActorIterator <ACameraActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		ActorItr->DetachFromActor(FDetachmentTransformRules(EDetachmentRule::KeepWorld, true));
		ActorItr->Destroy();
	}*/
	bool check_camera = (json_file_names.Num() == cameras_to_rebuild.Num()) && (cameras_to_rebuild[CurrentJsonFile].Len() == CameraConfigs.Num());
	for (int i = 0; i < CameraConfigs.Num(); ++i)
	{
		if (!check_camera || cameras_to_rebuild[CurrentJsonFile][i] == '1')
		{
			SpawnCamerasPlayback(CameraConfigs[i], 0);

			// Stereo
			if (CameraConfigs[i].StereoBaseline > 0.0)
			{
				SpawnCamerasPlayback(CameraConfigs[i], -1);
				SpawnCamerasPlayback(CameraConfigs[i], 1);
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
			for (FROXPawnInfo pawnInfo : PawnsInfo)
			{
				if (pawnInfo.PawnName == Itr->GetActorLabel())
				{
					Pawns.Add(*Itr);
				}
			}
		}
	}
}

void AROXTracker::SetCamerasAndRenderTargetsLocationAndRotation(int index, FVector NewLocation, FRotator NewRotation)
{
	if (retrieve_images_from_viewport)
	{
		CameraActors[index]->SetActorLocationAndRotation(NewLocation, NewRotation);
	}
	else
	{
		if (generate_rgb)
		{
			SceneCapture_Lit[index]->SetActorLocationAndRotation(NewLocation, NewRotation);
		}
		if (generate_object_mask)
		{
			SceneCapture_Mask[index]->SetActorLocationAndRotation(NewLocation, NewRotation);
		}
		if (generate_normal)
		{
			SceneCapture_Normal[index]->SetActorLocationAndRotation(NewLocation, NewRotation);
		}		
	}

	if (generate_depth)
	{
		SceneCapture_Depth[index]->SetActorLocationAndRotation(NewLocation, NewRotation);
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
		if (CameraActors.Num() > 1)
		{
			for (AROXBasePawn* p : Pawns)
			{
				p->CheckFirstPersonCamera(CameraActors[CurrentCamRebuildMode]);
			}
			ControllerPawn->ChangeViewTarget(CameraActors[CurrentCamRebuildMode]);
		}
	}
	ChangeViewmode(vm);

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateUObject(this, &AROXTracker::TakeScreenshotDelegate, vm), take_screenshot_delay, false);
}

void AROXTracker::TakeScreenshotDelegate(EROXViewMode vm)
{
	FTimerHandle TimerHandle;
	if (StereoCameraBaselines[CurrentCamRebuildMode] <= 0)
	{
		TakeScreenshotFolder(vm, CameraActors[CurrentCamRebuildMode]->GetActorLabel());
	}

	if (vm == EROXViewMode_Last || StereoCameraBaselines[CurrentCamRebuildMode] > 0)
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

void AROXTracker::CheckMaterialsAndTakeScreenshotRTDelegate(EROXViewMode vm)
{
	if (!retrieve_images_from_viewport &&
		((vm == EROXViewMode::RVM_ObjectMask && !isMaskedMaterial) ||
		((vm == EROXViewMode::RVM_Lit || vm == EROXViewMode::RVM_Normal) && isMaskedMaterial)))
	{
		ToggleActorMaterials();
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateUObject(this, &AROXTracker::TakeScreenshotsRenderTargetDelegate, vm), change_viewmode_delay, false);
	}
	else
	{
		TakeScreenshotsRenderTargetDelegate(vm);
	}
}

void AROXTracker::TakeScreenshotsRenderTargetDelegate(EROXViewMode vm)
{
	for (int i = 0; i < CameraActors.Num(); ++i)
	{
		CurrentCamRebuildMode = i;
		if (StereoCameraBaselines[i] <= 0)
		{
			for (AROXBasePawn* p : Pawns)
			{
				p->CheckFirstPersonCamera(CameraActors[i]);
			}

			TakeScreenshotFolder(vm, CameraActors[i]->GetActorLabel());
		}
	}

	
	if (vm != EROXViewMode_Last)
	{
		CheckMaterialsAndTakeScreenshotRTDelegate(NextViewmode(vm));
	}
	else
	{
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateUObject(this, &AROXTracker::RebuildModeMain), take_screenshot_delay, false);
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

	if (retrieve_images_from_viewport)
	{
		FROXObjectPainter::Get().PrintToJson(sceneObject_json_filename);
	}
	else
	{
		ROXJsonParser::WriteSceneObjects(MeshMaterials, sceneObject_json_filename);
	}

	JsonParser = new ROXJsonParser();
	JsonParser->LoadFile(scene_save_directory + scene_folder + "/" + json_file_names[CurrentJsonFile] + ".json");

	if (JsonParser->GetNumFrames() > 0)
	{
		CacheSceneActors(JsonParser->GetPawnsInfo(), JsonParser->GetCameraConfigs());
		DisableGravity();

		// Init viewtarget
		if (CameraActors.Num() > 0)
		{
			for (AROXBasePawn* p : Pawns)
			{
				p->CheckFirstPersonCamera(CameraActors[CurrentCamRebuildMode]);
			}
			ControllerPawn->ChangeViewTarget(CameraActors[CurrentCamRebuildMode]);
		}
	
		CurrentCamRebuildMode = 0;
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateUObject(this, &AROXTracker::RebuildModeMain), initial_delay, false);
	}
}

void AROXTracker::RebuildModeMain()
{
	if (numFrame < JsonParser->GetNumFrames())
	{
		// Set materials for the first viewmode
		if (!retrieve_images_from_viewport &&
			((EROXViewMode_First == EROXViewMode::RVM_ObjectMask && !isMaskedMaterial) ||
			((EROXViewMode_First == EROXViewMode::RVM_Lit || EROXViewMode_First == EROXViewMode::RVM_Normal) && isMaskedMaterial)))
		{
			ToggleActorMaterials();
		}

		int64 currentTime = FDateTime::Now().ToUnixTimestamp();
		PrintStatusToLog(start_frames[CurrentJsonFile], JsonReadStartTime, LastFrameTime, numFrame, currentTime, JsonParser->GetNumFrames());
		LastFrameTime = currentTime;

		if (!frame_already_loaded)
		{
			currentFrame = JsonParser->GetFrameData(numFrame);
		}
		else
		{
			frame_already_loaded = false;
		}

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

		if (!playback_only)
		{
			GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateUObject(this, &AROXTracker::RebuildModeMain_Camera), place_cameras_delay, false);
		}
		else
		{
			++numFrame;
			float frame_interval = 0.02;
			if (numFrame < JsonParser->GetNumFrames())
			{
				// Conversion from Ms to Segs
				float prevFrame_timestamp = currentFrame.time_stamp;
				currentFrame = JsonParser->GetFrameData(numFrame);
				frame_already_loaded = true;
				frame_interval = (currentFrame.time_stamp - prevFrame_timestamp) / playback_speed_rate / 1000;
			}
			
			GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateUObject(this, &AROXTracker::RebuildModeMain), frame_interval, false);
		}
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
	for (int i = 0; i < CameraActors.Num(); ++i)
	{
		FROXActorState* CamState = currentFrame.Cameras.Find(CameraActors[i]->GetActorLabel());
		if (CamState != nullptr)
		{
			SetCamerasAndRenderTargetsLocationAndRotation(i, CamState->Position, CamState->Rotation);

			// Stereo
			if (StereoCameraBaselines[i] > 0.0f && i + 2 < CameraActors.Num())
			{
				FRotator camera_rotation = FRotator(CamState->Rotation);

				FVector stereo_left_vector(0.0f, (-1)*StereoCameraBaselines[i]*0.5f, 0.0f);
				FVector stereo_left_vector_rot = camera_rotation.RotateVector(stereo_left_vector);
				SetCamerasAndRenderTargetsLocationAndRotation(i + 1, CamState->Position + stereo_left_vector_rot, CamState->Rotation);

				FVector stereo_right_vector(0.0f, StereoCameraBaselines[i]*0.5f, 0.0f);
				FVector stereo_right_vector_rot = camera_rotation.RotateVector(stereo_right_vector);
				SetCamerasAndRenderTargetsLocationAndRotation(i + 2, CamState->Position + stereo_right_vector_rot, CamState->Rotation);

				i = i + 2;
			}
		}
	}

	++numFrame;
	CurrentCamRebuildMode = 0;
	FTimerHandle TimerHandle;

	if (retrieve_images_from_viewport)
	{
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateUObject(this, &AROXTracker::ChangeViewmodeDelegate, EROXViewMode_First), first_viewmode_of_frame_delay, false);
	}
	else
	{
		if (RebuildView)
		{
			ControllerPawn->ChangeViewTarget(RebuildView);
		}
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateUObject(this, &AROXTracker::CheckMaterialsAndTakeScreenshotRTDelegate, EROXViewMode_First), first_viewmode_of_frame_delay, false);
	}
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
		if (!playback_only)
		{
			status_msg += " - Estimated Remaining Time: " + SecondsToString(remainingTimeSec) + " - Last Frame Time: " + FString::FromInt(lastFrameElapsedTimeSec) + "sec - Total Elapsed Time: " + SecondsToString(elapsedTimeSec);
		}

		UE_LOG(LogUnrealROX, Warning, TEXT("%s"), *status_msg);
	}
}

void AROXTracker::SetRecordSettings()
{
	for (TActorIterator <ALight> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		//FString fullName = Itr->GetFullName();
		//UE_LOG(LogUnrealROX, Warning, TEXT("%s"), *fullName);
		ActorItr->Destroy();
	}
}