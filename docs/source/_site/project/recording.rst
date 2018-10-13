*********
Recording
*********

During the recording process all scene information such as, camera location, its configuration and objects pose and orientation, among others, are dumped in a *.txt* file which is stored by default on the *RecordedSequences* folder located in the root of the UnrealROX project. Recording process is the same regardless of used VR hardware. VR controllers have their own input map, so there are different instructions for each of the used hardware. 

Configure HMD position
######################

Before recording you need to check for the **HMD position**. You need to check if your HMD and its attached camera are in the correct position. This is variable according to user's height, so, you need to manually configure your main camera position (the main camera is what you see on your Oculus headset). For the configuration [#f1]_:

	- **with OculusVR**: you need to press *left joystick* to enable HMD position changes. You are now able to change camera position along Z axis using right joystick and also camera translation on X and Y axes with the *left joystick*. A good tip for adjusting camera position is checking if shoulders position is corresponding with user's real shoulder position. User's should also be able to see its chest by looking down.

	- **with HTC Vive PRO**: TODO


Begin/Stop recording
####################

To begin/stop recording, user need to:

- **with OculusVR**: press *A button* from the right controller. Press again to stop recording.

- **with HTC Vive PRO**: TODO


A red collored message will appear both on the screen and HMD when recording a sequence. Once recording process is stopped, a *scene.txt* file will be created with all scene information. 



With the above instructions you will be able to record your own dataset!



.. rubric: Footnotes

.. [#f1] Once HMD position is configured, restarting the scene using VR controllers wouldn't change this configuration. 