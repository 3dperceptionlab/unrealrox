********
Playback
********

With this final step you will generate the ground truth (e.g. semantic segmentation and depth maps, normal maps, stereo pairs, and also instance segmentation, etc.). For your recorded sequence. First of all, you need to convert the .txt file which is the output of recording module to JSON format which is the input to playback module. For this purpose, you can use the script *scenetojson.py* which will perfectly do this step for you. You also will get information such as, sequence length in seconds, fps, and total frames. 

In order to proceed with the playback process, you will need to uncheck *Record mode*. You need to search ROXTracker in the *World Outlier* and locate "Recording configuration section". For a more detailed information, see *UnrealROX->Recording* section. After this brief configuration you can run the process in the *Selected Viewport* mode. All the data will be saved by default on GeneratedSequences folder located in the root of UnrealROX project.

Note: If your main purpose is to generate data and you run the project in *VR Preview* mode, UnrealROX wouldn't work properly.