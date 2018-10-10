************
Installation
************

.. _Unreal Engine: https://www.unrealengine.com
.. _Oculus SDK for Windows: https://developer.oculus.com/downloads/package/oculus-sdk-for-windows/
.. _Vive Software: https://www.vive.com/us/setup/vive/

First of all and before installation, you will need to statisfy some hardware and software prerequisites. 

Hardware and software prerequisites
###################################

`UnrealROX github`_


1. **Software**:

	1.1 **Unreal Engine 4.18**. You will need to download and install `Unreal Engine`_ 4.18. To access UE4 download you need to firstly create an Epic Games account. Note that UnrealROX was developed using Unreal Engine 4.18, so maybe wouldn't work properly with other UE versions.

	1.2 **VR software**. You have to install either the Oculus or HTC Vive Pro software, or both.
		1.2.1 **Oculus SDK**. Download and install `Oculus SDK for Windows`_. Oculus SDK version used for the development of UnrealROX was 1.30.0 and published on 08/10/2018. 
		1.2.2 **HTC Vive Pro SDK**. Download and install `Vive Software`_. 

	1.2 **Visual Studio**. In order to compile UnrealROX you will need Visual Studio. We recommend to launch Unreal Engine from Visual Studio selecting *robotrix* as starting point (important to avoid compiling the entire engine) and compiling the project in *Development editor* mode for a better performance.  


2. **Hardware**:

	2.1. **GPU**. Make sure your GPU driver is well installed and updated. You need a good GPU to run smoothly a photorealistic scene alongside UnrealROX system.

	2.2 **Overall hardware requirements**. For a smooth experience in photorealistic virtual environments rendered by Unreal Engine we recommend a good performance hardware configuration. 

	2.3 **VR headset**. Check if Oculus VR and/or HTC Vive PRO perform properly and if their installation and calibration was done correctly to achieve a good tracking.

3. **You are ready for the next step!**

How to import the project in a new UE4 project. Step-by-step installation.


How to import your scene to UnrealROX
#####################################

The straightforward way to use UnrealROX is importing your own scene to the project. To correctly integrate your project with UnrealROX you need to proceed with the following steps:

1. **Get UnrealROX**. Clone or download `UnrealROX github`_.

2. **Migrate your scene**. On the one hand we have the UnrealROX project to which we want to import the scene from other UE4 project. In the Unreal Engine editor of the scene project, go to the Content Browser and localize *.umap* file of your scene, *right-click* and go to *Asset Actions->Migrate*. Now you need to navigate to the *Content* folder of UnrealROX project and select it as target for migrating your scene. 

3. **Put the robot in your scene**. The last step is to put the robot in your scene. From this point we will only work on the UnrealROX project. In the UE4 editor go to the Content Browser and navigate to *Mannequin->Meshes* selecting your pawn (e.g. *ROXMannequinPawn*) and drag it to the scene. You can scale your robot to match it with your scene scale.

4. **You are ready for the next step!**