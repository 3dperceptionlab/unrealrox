**********
Quickstart
**********

.. _pivot tool: https://www.unrealengine.com/marketplace/pivot-tool
.. _UnrealROX github: https://github.com/3dperceptionlab/unrealrox

The main purpose of this tutorial is teaching you to successfully use UnrealROX in your scene [#f1]_.

First of all, please check and satisfy all the :ref:`requirements`. Now in order to get started with UnrealROX you need to perform the following steps:


How to import your scene to UnrealROX
#####################################

The straightforward way to use UnrealROX is importing your own scene to the project. To correctly integrate your project with UnrealROX you need to proceed with the following steps:

1. **Get UnrealROX**. Clone or download `UnrealROX github`_.

2. **Migrate your scene**. On the one hand we have the UnrealROX project to which we want to import the scene from other UE4 project. In the Unreal Engine editor of the scene project, go to the Content Browser and localize *.umap* file of your scene, *right-click* and go to *Asset Actions->Migrate*. Now you need to navigate to the *Content* folder of UnrealROX project and select it as target for migrating your scene. 

3. **Put the robot in your scene**. The last step is to put the robot in your scene. From this point we will only work on the UnrealROX project. In the UE4 editor go to the Content Browser and navigate to *Mannequin->Meshes* selecting your pawn (e.g. *ROXMannequinPawn*) and drag it to the scene. You can scale your robot to match it with your scene scale.

4. **You are ready for the next step!**


How to configure your scene
###########################

Once your scene is integrated with UnrealROX project, you need to do the following steps:

1. **Impress your boss**. First of all you need a UE4 scene as photorealistic as possible to impress your boss!

2. **Check object pivots**. Run your scene and check that all object pivots are placed on the geometrical center of its meshes for configure X and Y axis (Note: Z axis should be 0 for a better interaction with some objects). This is important to track all the objects correctly during the recording and playback steps. However, this is a tedious task thus realistic scenes have lots of objects. Due to this, we use the plugin `pivot tool`_ which works like a charm (we don't include it in the project because of license conflicts).

3. **Configure interactable objects**. Choose the object you want to interact with from the World Outliner (by default placed on the right side in the UE4 editor) and do the following:

	3.1. **You need object to be movable**. Go to *Transform->Mobility* and set the object to *Movable*. Almost all the objects are static by default.

	3.2. **You need physics simulation**. Go to *Physics* and check *Simulate Physics* option.
	
	3.3. **You need overlap events**. Go to *Collision* and check *Generate Overlap Events* option. **This is a must** for the grasping system in order to grab an object correctly. If this option is disabled, grasping wouldn't work.
	
	3.4. **You need an accurate collision mesh**. You also need to check object geometry to achieve a visually plausible grasping. *Right-click* on the object and *Edit*. In the *Object Editor* you need to visualize the simple and complex collision meshes and visually check its accuracy. If object geometry is complex and the collision mesh is rough, you should improve this by auto generating convex collision (go to editor menubar and *Collision->Auto Convex Collision*) with maximum hull verts and accuracy. For the objects with a complex geometry you should set the *Collision Complexity* to *Use simple collision as complex*. In this way you will achieve a more realistic grasping, however, physics simulation will be much more complicated so when interacting with two or more objects you may notice an unstable behavior.

4. **Build scene lighting**. Now you need to build project lighting! The steps above are very important, especially the step of setting the objects as movable. You need to set lighting configuration according to your PC specs. By default, lighting configuration is too demanding for a mid-high range computer. For a fast and feasible lighting generation we recommend the following configuration you can do in the *World Settings->Lightmass*. See the Figure 1.


.. figure:: /_static/lightmass.jpg
    :scale: 75 %
    :align: center
    :alt: Scene light configuration we used to build lighting and produce photorealistic results.
    :figclass: align-center

    Figure 1. Scene light configuration we used to build lighting and produce photorealistic results.

5. **Scene ready to record your own sequences!**



OculusVR and HTC Vive PRO controllers
#####################################

In this section we will describe the input map for OculusVR and HTC Vive PRO controllers. Figure 2 is a representation of Oculus controllers with the function of each button.

.. figure:: /_static/oculus_controllers.png
    :scale: 20 %
    :align: center
    :alt: Oculus controller representation with the correspondence between the robot actions and cotroller buttons.
    :figclass: align-center

    Figure 2. Oculus controllers representation with the correspondence between the robot actions and cotroller buttons.

Using:

- **Left and Right Joysticks**: 
	- to move and orient the robot in the scene
	- (by first pressing the left joystick) user will be able to position the first person camera according to its height. Use right joystick to move on Z axis and left joystick for movement on X and Y axes.

- **Left and Right Grasp**: grab an object with the left or right hand correspondly.

- **Y button**: restart the level placing all the objects to its initial position. First person camera configuration is mantained.

- **X button**: reset VR changing first person camera to its default position and configuration

- **B button**: turn ON/OFF HUD used for debugging purposes. It enables a mirror to see better robot head position while configuring first person camera

- **A button**: begin/stop recording process which will dump all the scene information to a .txt file. This file will be used for the playback process.

- **Unused buttons**: oculus button and left and right triggers.


TODO: HTC Vive PRO controllers


*Note*: Robot head is attached to the VR headset tracking user's head position. This entails some problems such as, user's height. You will need to configure camera position according with your height before recording.


Record your own dataset
#######################

Acomplishing with above instructions you will be able now to record your own dataset. Scene information (e.g. camera and objects pose and orientation) is dumped using .txt format and stored by default on the RecordedSequences folder located in the root of the UnrealROX project. When you are recording, a red message will appear on the screen. You can begin and stop recording in each moment. When you restart the level, recording process automatically stops. For more information go to *UnrealROX Plugin->Recording* section.


Generate ground truth for your record
#####################################

This is the final step of this tutorial. Here you will generate the ground truth (e.g. semantic segmentation and depths maps, normal maps, stereo pairs, and also instance segmentation, etc.) for your recorded sequence. First of all, you need to convert the .txt file which is the output of recording module to JSON format which is the input to playback module. For this purpose, you can use the script *scenetojson.py* which will perfectly do this step for you. You also will get information such as, sequence length in seconds, fps, and total frames. 

In order to proceed with the playback process, you will need to uncheck *Record mode*. You need to search ROXTracker in the *World Outliner* and locate "Recording configuration section". For a more detailed information, see *UnrealROX->Recording* section. After this brief configuration you can run the process in the *Selected Viewport* mode. All the data will be saved by default on GeneratedSequences folder located in the root of UnrealROX project.

Note: If your main purpose is to generate data and you run the project in *VR Preview* mode, UnrealROX wouldn't work properly.


"That's all Folks!"


.. rubric: Footnotes

.. [#f1] this tutorial was done with Unreal Engine 4.18. We cannot guarantee UnrealROX work properly with other UE version.