// Fill out your copyright notice in the Description page of Project Settings.

#include "ROXJsonParser.h"

ROXJsonParser::ROXJsonParser()
	: NumFrames(0)
	, SequenceName("")
{

}

ROXJsonParser::~ROXJsonParser()
{
}


bool ROXJsonParser::LoadFile(FString JsonFilePath)
{
	bool file_loaded = false;
	FString JsonRaw;
	file_loaded = FFileHelper::LoadFileToString(JsonRaw, *JsonFilePath);

	if (file_loaded)
	{
		//Create a json object to store the information from the json string
		//The json reader is used to deserialize the json object later on
		TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(JsonRaw);

		if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
		{
			NumFrames = JsonObject->GetIntegerField("total_frames");
			SequenceName = JsonObject->GetStringField("name");
			TotalTime = JsonObject->GetNumberField("total_time");
			MeanFramerate = JsonObject->GetNumberField("mean_framerate");

			CamerasJsonArray = JsonObject->GetArrayField("cameras");
			PawnsJsonArray = JsonObject->GetArrayField("skeletons");
			FramesJsonArray = JsonObject->GetArrayField("frames");

			TSharedPtr<FJsonObject> CurrentCameraObject;
			for (int32 i = 0; i < CamerasJsonArray.Num(); ++i)
			{
				CurrentCameraObject = CamerasJsonArray[i]->AsObject();
				CameraNames.Add(CurrentCameraObject->GetStringField("name"));
			}

			TSharedPtr<FJsonObject> CurrentPawnObject;
			for (int32 i = 0; i < PawnsJsonArray.Num(); ++i)
			{
				CurrentPawnObject = PawnsJsonArray[i]->AsObject();
				PawnNames.Add(CurrentPawnObject->GetStringField("name"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("JSON couldn't be deserialized."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("JSON couldn't be read."));
	}

	return file_loaded;
}

FROXFrame ROXJsonParser::GetFrameData(uint64 nFrame)
{
	FROXFrame Frame;
	TSharedPtr<FJsonObject> FrameObject = FramesJsonArray[nFrame]->AsObject();

	Frame.time_stamp = FrameObject->GetNumberField("timestamp");
	Frame.n_frame = FrameObject->GetIntegerField("id");

	// Objects (StaticMesh)
	TArray<TSharedPtr<FJsonValue>> ObjectsJson = FrameObject->GetArrayField("objects");
	for (TSharedPtr<FJsonValue> ObjectJson : ObjectsJson)
	{
		TSharedPtr<FJsonObject> ObjectObject = ObjectJson->AsObject();

		TSharedPtr<FJsonObject> PositionObject = ObjectObject->GetObjectField("position");
		TSharedPtr<FJsonObject> RotationObject = ObjectObject->GetObjectField("rotation");
		TSharedPtr<FJsonObject> BBMinObject = ObjectObject->GetObjectField("boundingbox_min");
		TSharedPtr<FJsonObject> BBMaxObject = ObjectObject->GetObjectField("boundingbox_max");
		FString ObjName = ObjectObject->GetStringField("name");

		FROXActorStateExtended ObjectState;
		ObjectState.Position = FVector(PositionObject->GetNumberField("x"), PositionObject->GetNumberField("y"), PositionObject->GetNumberField("z"));
		ObjectState.Rotation = FRotator(RotationObject->GetNumberField("p"), RotationObject->GetNumberField("y"), RotationObject->GetNumberField("r"));
		ObjectState.BoundingBox_Min = FVector(BBMinObject->GetNumberField("x"), BBMinObject->GetNumberField("y"), BBMinObject->GetNumberField("z"));
		ObjectState.BoundingBox_Max = FVector(BBMaxObject->GetNumberField("x"), BBMaxObject->GetNumberField("y"), BBMaxObject->GetNumberField("z"));		

		Frame.Objects.Add(ObjName, ObjectState);
	}

	// Cameras (CameraActor)
	TArray<TSharedPtr<FJsonValue>> CamerasJson = FrameObject->GetArrayField("cameras");
	for (TSharedPtr<FJsonValue> CameraJson : CamerasJson)
	{
		TSharedPtr<FJsonObject> CameraObject = CameraJson->AsObject();

		TSharedPtr<FJsonObject> PositionObject = CameraObject->GetObjectField("position");
		TSharedPtr<FJsonObject> RotationObject = CameraObject->GetObjectField("rotation");
		FString CamId = CameraObject->GetStringField("name");

		FROXActorState CameraState;
		CameraState.Position = FVector(PositionObject->GetNumberField("x"), PositionObject->GetNumberField("y"), PositionObject->GetNumberField("z"));
		CameraState.Rotation = FRotator(RotationObject->GetNumberField("p"), RotationObject->GetNumberField("y"), RotationObject->GetNumberField("r"));

		Frame.Cameras.Add(CamId, CameraState);
	}

	// Skeletons (ROXBasePawns)
	TArray<TSharedPtr<FJsonValue>> SkeletonsJson = FrameObject->GetArrayField("skeletons");
	for (TSharedPtr<FJsonValue> SkeletonJson : SkeletonsJson)
	{
		TSharedPtr<FJsonObject> SkeletonObject = SkeletonJson->AsObject();

		TSharedPtr<FJsonObject> PositionObject = SkeletonObject->GetObjectField("position");
		TSharedPtr<FJsonObject> RotationObject = SkeletonObject->GetObjectField("rotation");
		FString SkName = SkeletonObject->GetStringField("name");

		FROXSkeletonState SkeletonState;
		SkeletonState.Position = FVector(PositionObject->GetNumberField("x"), PositionObject->GetNumberField("y"), PositionObject->GetNumberField("z"));
		SkeletonState.Rotation = FRotator(RotationObject->GetNumberField("p"), RotationObject->GetNumberField("y"), RotationObject->GetNumberField("r"));

		// Bones
		TArray<TSharedPtr<FJsonValue>> BonesJson = SkeletonObject->GetArrayField("bones");
		for (TSharedPtr<FJsonValue> BoneJson : BonesJson)
		{
			TSharedPtr<FJsonObject> BoneObject = BoneJson->AsObject();

			TSharedPtr<FJsonObject> BonePositionObject = BoneObject->GetObjectField("position");
			TSharedPtr<FJsonObject> BoneRotationObject = BoneObject->GetObjectField("rotation");
			FString BoneName = BoneObject->GetStringField("name");

			FROXActorState BoneState;
			BoneState.Position = FVector(BonePositionObject->GetNumberField("x"), BonePositionObject->GetNumberField("y"), BonePositionObject->GetNumberField("z"));
			BoneState.Rotation = FRotator(BoneRotationObject->GetNumberField("p"), BoneRotationObject->GetNumberField("y"), BoneRotationObject->GetNumberField("r"));

			SkeletonState.Bones.Add(BoneName, BoneState);
		}

		Frame.Skeletons.Add(SkName, SkeletonState);
	}

	return Frame;
}

FString ROXJsonParser::IntToStringDigits(int i, int nDigits)
{
	FString res(FString::FromInt(i));
	if (nDigits > res.Len())
	{
		FString zeroes("");
		int remainingZeros = nDigits - res.Len();
		for (int j = 0; j < remainingZeros; ++j)
		{
			zeroes += "0";
		}
		res = zeroes + res;

		if (res.Len() != nDigits)
		{
			res = FString::FromInt(i);
		}
	}
	return res;
}

TSharedPtr<FJsonObject> JsonObjectXYZ(float x, float y, float z)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	JsonObject->SetNumberField("x", x);
	JsonObject->SetNumberField("y", y);
	JsonObject->SetNumberField("z", z);
	return JsonObject;
}

TSharedPtr<FJsonObject> JsonObjectPitchYawRoll(float pitch, float yaw, float roll)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	JsonObject->SetNumberField("p", pitch);
	JsonObject->SetNumberField("y", yaw);
	JsonObject->SetNumberField("r", roll);
	return JsonObject;
}

float FloatTxt(FString txt_string)
{
	float res(0.0f);
	TArray<FString> float_parts;
	txt_string.ParseIntoArray(float_parts, TEXT("="));
	if (float_parts.Num() > 1)
	{
		res = FCString::Atof(*float_parts[1]);
	}
	return res;
}

void ROXJsonParser::SceneTxtToJson(FString path, FString txt_filename, FString json_filename)
{
	FString txt_file;
	FString txt_file_path = path + "/" + txt_filename + ".txt";
	bool fileLoaded = FFileHelper::LoadFileToString(txt_file, *txt_file_path);

	if (fileLoaded)
	{
		TArray<FString> txt_file_lines;
		txt_file.ParseIntoArray(txt_file_lines, TEXT("\n"));

		// Get cameras
		TArray<TSharedPtr<FJsonValue>> JsonArray_SequenceCameras;

		int startLine_Cameras = 0;
		TArray<FString> line_numCameras;
		txt_file_lines[startLine_Cameras].ParseIntoArray(line_numCameras, TEXT(" "));
		int numCameras = FCString::Atoi(*line_numCameras[1]);

		for (int i = startLine_Cameras + 1; i < startLine_Cameras + numCameras + 1; ++i)
		{
			TArray<FString> line_camera_header;
			txt_file_lines[i].ParseIntoArray(line_camera_header, TEXT(" "));

			TSharedPtr<FJsonObject> Json_Camera = MakeShareable(new FJsonObject());
			Json_Camera->SetStringField("name", line_camera_header[0]);
			Json_Camera->SetNumberField("stereo", FCString::Atof(*line_camera_header[1]));
			Json_Camera->SetNumberField("fov", FCString::Atof(*line_camera_header[2]));
			JsonArray_SequenceCameras.Add(MakeShareable(new FJsonValueObject(Json_Camera)));
		}

		// Get Objects
		int startLine_Objects = startLine_Cameras + 1 + numCameras;
		TArray<FString> line_numObjects;
		txt_file_lines[startLine_Objects].ParseIntoArray(line_numObjects, TEXT(" "));
		int numObjects = FCString::Atoi(*line_numObjects[1]);

		// GetSkeletons
		TArray<TSharedPtr<FJsonValue>> JsonArray_SequenceSkeletons;
		int startLine_Skeletons = startLine_Objects + 1;
		TArray<FString> line_numSkeletons;
		txt_file_lines[startLine_Skeletons].ParseIntoArray(line_numSkeletons, TEXT(" "));
		int numSkeletons = FCString::Atoi(*line_numSkeletons[1]);
		TArray<int> numBonesSkeletons;

		for (int i = startLine_Skeletons + 1; i < startLine_Skeletons + numSkeletons + 1; ++i)
		{
			TArray<FString> line_skeleton_header;
			txt_file_lines[i].ParseIntoArray(line_skeleton_header, TEXT(" "));

			TSharedPtr<FJsonObject> Json_Skeleton = MakeShareable(new FJsonObject());
			Json_Skeleton->SetStringField("name", line_skeleton_header[0]);
			int numBonesSkeleton = FCString::Atoi(*line_skeleton_header[1]);
			numBonesSkeletons.Add(numBonesSkeleton);
			Json_Skeleton->SetNumberField("num_bones", numBonesSkeleton);
			JsonArray_SequenceSkeletons.Add(MakeShareable(new FJsonValueObject(Json_Skeleton)));
		}

		// Get Non Movable Objects
		TArray<TSharedPtr<FJsonValue>> JsonArray_SequenceNonMovable;
		int startLine_NonMovable = startLine_Skeletons + numSkeletons + 1;
		TArray<FString> line_numNonMovable;
		txt_file_lines[startLine_NonMovable].ParseIntoArray(line_numNonMovable, TEXT(" "));
		int numNonMovable = FCString::Atoi(*line_numNonMovable[1]);

		for (int i = startLine_NonMovable + 1; i < startLine_NonMovable + numNonMovable + 1; ++i)
		{
			TArray<FString> line_nm;
			txt_file_lines[i].ParseIntoArray(line_nm, TEXT(" "));

			TSharedPtr<FJsonObject> Json_NonMovableObject = MakeShareable(new FJsonObject());
			Json_NonMovableObject->SetStringField("name", line_nm[0]);
			Json_NonMovableObject->SetObjectField("position", JsonObjectXYZ(FloatTxt(line_nm[1]), FloatTxt(line_nm[2]), FloatTxt(line_nm[3])));
			Json_NonMovableObject->SetObjectField("rotation", JsonObjectPitchYawRoll(FloatTxt(line_nm[4]), FloatTxt(line_nm[5]), FloatTxt(line_nm[6])));
			Json_NonMovableObject->SetObjectField("boundingbox_min", JsonObjectXYZ(FloatTxt(line_nm[7]), FloatTxt(line_nm[8]), FloatTxt(line_nm[9])));
			Json_NonMovableObject->SetObjectField("boundingbox_max", JsonObjectXYZ(FloatTxt(line_nm[10]), FloatTxt(line_nm[11]), FloatTxt(line_nm[12])));
			JsonArray_SequenceNonMovable.Add(MakeShareable(new FJsonValueObject(Json_NonMovableObject)));
		}

		// Get Frame		
		TArray<TSharedPtr<FJsonValue>> JsonArray_SequenceFrames;

		int startLine_Frames = startLine_NonMovable + numNonMovable + 1;
		int c_line = startLine_Frames;
		int numFrames = 0;
		float firstFrameTimestamp = 0.0f;
		float totalTime = 0.0f;
		int numLinesPerFrame = 2 + (numCameras)+(1 + numObjects) + (1 + numSkeletons);
		for (int i = 0; i < numSkeletons; ++i)
		{
			numLinesPerFrame += (numBonesSkeletons[i]);
		}
		int nEstimatedFrames = (txt_file_lines.Num() - c_line) / numLinesPerFrame;

		while (c_line + numLinesPerFrame <= txt_file_lines.Num())
		{
			TSharedPtr<FJsonObject> Json_Frames = MakeShareable(new FJsonObject());

			// frame
			c_line++;
			TArray<FString> line_id_frame;
			txt_file_lines[c_line].ParseIntoArray(line_id_frame, TEXT(" "));
			Json_Frames->SetStringField("id", IntToStringDigits(FCString::Atoi(*line_id_frame[0]), 6));
			float timestamp = FCString::Atof(*line_id_frame[1]);
			Json_Frames->SetNumberField("timestamp", timestamp);

			if (numFrames == 0)
			{
				firstFrameTimestamp = timestamp;
			}
			else
			{
				totalTime = timestamp - firstFrameTimestamp;
			}
			numFrames++;

			// Cameras
			TArray<TSharedPtr<FJsonValue>> JsonArray_FrameCameras;
			for (int i = 0; i < numCameras; ++i)
			{
				c_line++;
				TArray<FString> line_cf;
				txt_file_lines[c_line].ParseIntoArray(line_cf, TEXT(" "));
				TSharedPtr<FJsonObject> Json_FrameCamera = MakeShareable(new FJsonObject());
				Json_FrameCamera->SetStringField("name", line_cf[0]);
				Json_FrameCamera->SetObjectField("position", JsonObjectXYZ(FloatTxt(line_cf[1]), FloatTxt(line_cf[2]), FloatTxt(line_cf[3])));
				Json_FrameCamera->SetObjectField("rotation", JsonObjectPitchYawRoll(FloatTxt(line_cf[4]), FloatTxt(line_cf[5]), FloatTxt(line_cf[6])));
				JsonArray_FrameCameras.Add(MakeShareable(new FJsonValueObject(Json_FrameCamera)));
			}
			Json_Frames->SetArrayField("cameras", JsonArray_FrameCameras);

			c_line++;
			// objects
			TArray<TSharedPtr<FJsonValue>> JsonArray_FrameObjects;
			TArray<FString> line_of;
			for (int i = 0; i < numObjects; ++i)
			{
				c_line++;
				line_of.Empty();
				txt_file_lines[c_line].ParseIntoArray(line_of, TEXT(" "));
				TSharedPtr<FJsonObject> Json_FrameObject = MakeShareable(new FJsonObject());
				Json_FrameObject->SetStringField("name", line_of[0]);
				Json_FrameObject->SetObjectField("position", JsonObjectXYZ(FloatTxt(line_of[1]), FloatTxt(line_of[2]), FloatTxt(line_of[3])));
				Json_FrameObject->SetObjectField("rotation", JsonObjectPitchYawRoll(FloatTxt(line_of[4]), FloatTxt(line_of[5]), FloatTxt(line_of[6])));
				Json_FrameObject->SetObjectField("boundingbox_min", JsonObjectXYZ(FloatTxt(line_of[7]), FloatTxt(line_of[8]), FloatTxt(line_of[9])));
				Json_FrameObject->SetObjectField("boundingbox_max", JsonObjectXYZ(FloatTxt(line_of[10]), FloatTxt(line_of[11]), FloatTxt(line_of[12])));
				JsonArray_FrameObjects.Add(MakeShareable(new FJsonValueObject(Json_FrameObject)));
			}
			Json_Frames->SetArrayField("objects", JsonArray_FrameObjects);

			c_line++;
			// skeletons
			TArray<TSharedPtr<FJsonValue>> JsonArray_FrameSkeletons;
			TArray<FString> line_sf;
			TArray<FString> line_bsf;
			for (int i = 0; i < numSkeletons; ++i)
			{
				c_line++;
				line_sf.Empty();
				txt_file_lines[c_line].ParseIntoArray(line_sf, TEXT(" "));
				TSharedPtr<FJsonObject> Json_FrameSkeleton = MakeShareable(new FJsonObject());
				Json_FrameSkeleton->SetStringField("name", line_sf[0]);
				Json_FrameSkeleton->SetObjectField("position", JsonObjectXYZ(FloatTxt(line_sf[1]), FloatTxt(line_sf[2]), FloatTxt(line_sf[3])));
				Json_FrameSkeleton->SetObjectField("rotation", JsonObjectPitchYawRoll(FloatTxt(line_sf[4]), FloatTxt(line_sf[5]), FloatTxt(line_sf[6])));

				TArray<TSharedPtr<FJsonValue>> JsonArray_FrameSkeletonBones;
				for (int j = 0; j < numBonesSkeletons[i]; ++j)
				{
					c_line++;
					line_bsf.Empty();
					txt_file_lines[c_line].ParseIntoArray(line_bsf, TEXT(" "));
					TSharedPtr<FJsonObject> Json_FrameSkeletonBone = MakeShareable(new FJsonObject());
					Json_FrameSkeletonBone->SetStringField("name", line_bsf[0]);
					Json_FrameSkeletonBone->SetObjectField("position", JsonObjectXYZ(FloatTxt(line_bsf[1]), FloatTxt(line_bsf[2]), FloatTxt(line_bsf[3])));
					Json_FrameSkeletonBone->SetObjectField("rotation", JsonObjectPitchYawRoll(FloatTxt(line_bsf[4]), FloatTxt(line_bsf[5]), FloatTxt(line_bsf[6])));
					JsonArray_FrameSkeletonBones.Add(MakeShareable(new FJsonValueObject(Json_FrameSkeletonBone)));
				}
				Json_FrameSkeleton->SetArrayField("bones", JsonArray_FrameSkeletonBones);
				JsonArray_FrameSkeletons.Add(MakeShareable(new FJsonValueObject(Json_FrameSkeleton)));

			}
			Json_Frames->SetArrayField("skeletons", JsonArray_FrameSkeletons);

			JsonArray_SequenceFrames.Add(MakeShareable(new FJsonValueObject(Json_Frames)));
			c_line++;
		}

		totalTime = totalTime / 1000.0f;
		TSharedPtr<FJsonObject> JsonSequence = MakeShareable(new FJsonObject);
		JsonSequence->SetStringField("name", json_filename);
		JsonSequence->SetNumberField("total_frames", numFrames);
		JsonSequence->SetNumberField("total_time", totalTime);
		JsonSequence->SetNumberField("mean_framerate", numFrames / totalTime);
		JsonSequence->SetArrayField("cameras", JsonArray_SequenceCameras);
		JsonSequence->SetArrayField("skeletons", JsonArray_SequenceSkeletons);
		JsonSequence->SetArrayField("non_movable_objects", JsonArray_SequenceNonMovable);
		JsonSequence->SetArrayField("frames", JsonArray_SequenceFrames);

		// Write JSON file
		FString OutputString;
		TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(JsonSequence.ToSharedRef(), Writer);
		FString json_file_path = path + "/" + json_filename + ".json";
		FFileHelper::SaveStringToFile(OutputString, *json_file_path, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), EFileWrite::FILEWRITE_None);

		FString success_message("Scene JSON file named " + json_filename + ".json has been created successfully. Frames: " + FString::FromInt(numFrames) + ". Total time: " + FString::SanitizeFloat(totalTime) + ". Mean framerate: " + FString::SanitizeFloat(numFrames / totalTime));
		UE_LOG(LogTemp, Warning, TEXT("%s"), *success_message);
	}
	else
	{
		FString error_message("Scene TXT file named " + txt_filename + ".txt does not exist.");
		UE_LOG(LogTemp, Warning, TEXT("%s"), *error_message);
	}
}