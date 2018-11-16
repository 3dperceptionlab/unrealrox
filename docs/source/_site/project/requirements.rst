.. _requirements:

***********************************
Hardware and Software prerequisites
***********************************


.. _Unreal Engine official instalation guide: https://docs.unrealengine.com/en-us/GettingStarted/Installation
.. _Unreal Engine get source code guide: https://docs.unrealengine.com/en-us/GettingStarted/DownloadingUnrealEngine
.. _Oculus SDK for Windows: https://developer.oculus.com/downloads/package/oculus-sdk-for-windows/
.. _Vive Software: https://www.vive.com/us/setup/vive/
.. _Line 982: https://github.com/EpicGames/UnrealEngine/blob/4.18/Engine/Source/Runtime/Engine/Private/StaticMeshRender.cpp#L982
.. _line 1020: https://github.com/EpicGames/UnrealEngine/blob/4.19/Engine/Source/Runtime/Engine/Private/StaticMeshRender.cpp#L1020
.. _line 1063: https://github.com/EpicGames/UnrealEngine/blob/4.20/Engine/Source/Runtime/Engine/Private/StaticMeshRender.cpp#L1063

First of all and before installation, you will need to statisfy some hardware and software prerequisites:

1. **Software**:
	For the development of UnrealROX we used Windows 10 OS. UnrealROX was not tested on Linux or MacOS. Software requirements are the following:

	1.1 **VR software**. You have to install either the Oculus or HTC Vive Pro software, or both.
		
		1.1.1 **Oculus SDK**. Download and install `Oculus SDK for Windows`_. Oculus SDK version used for the development of UnrealROX was 1.30.0 and published on 08/10/2018. 
		
		1.1.2 **HTC Vive Pro SDK**. Download and install `Vive Software`_. 

	1.2 **Visual Studio**. In order to compile UnrealROX and Unreal Engine you will need Visual Studio. For this purpose you need to install Visual Studio 2017 with the configuration file we provide (put link). In the Visual Studio Installer you can import a VS configuration file and all required individual packages and workloads will be automatically installed. This is important in order to avoid some errors when compiling UnrealROX software or different versions of Unreal Engine.

	1.3 **Unreal Engine 4.18 or higher**. UnrealROX was originally implemented using UE 4.18 version (implementation on 4.19 and 4.20 comming soon). UE4 installation can be done in two ways depending if you need the mask segmentations of your recorded data. 

		1.3.1 **I don't need mask segmentations**. In this case you will only need to follow `Unreal Engine official instalation guide`_ and install the compatible UE4 version with UnrealROX.

		1.3.2 **I need mask segmentations**. In order to get mask segmentations from UE4 you need to recompile the complete engine from scratch. The first step is to get the source code for your UE4 version following the `Unreal Engine get source code guide`_ (you should use the release branch in the Unreal Engine github repo). In the aforementioned guide there are also compilation instructions depending on your operating system. Before compiling the engine do the following:

			- Go to */Engine/Source/Runtime/Engine/Private/StaticMeshRender.cpp* and search for *if (bProxyIsSelected && EngineShowFlags.VertexColors && AllowDebugViewmodes())* condition (`Line 982`_ for UE 4.18, `line 1020`_ for UE 4.19 and `line 1063`_ for UE 4.20). 

			- Remove **bProxyIsSelected** flag. The presence of this flag disables the visualization of per vertex coloring. 

			- Compile engine code with Visual Studio 2017 or higher. 


2. **Hardware**:

	2.1. **GPU**. Make sure your GPU driver is well installed and updated. You need a good GPU to run smoothly a photorealistic scene alongside UnrealROX system. We used a Titan X GPU.

	2.2 **Overall hardware requirements**. For a smooth experience in photorealistic virtual environments rendered by Unreal Engine we recommend a good performance hardware configuration. 

	2.3 **VR headset**. Check if Oculus VR and/or HTC Vive PRO perform properly and if their installation and calibration was done correctly to achieve a good tracking.

3. **You are now ready to install UnrealROX, import your photorrealistic scene and create your own awesome dataset!**



