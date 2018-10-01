*******
Welcome
*******

What is UnrealROX?
==================

UnrealROX is an extremely photorealistic virtual reality environment built over Unreal Engine 4 and designed for generating synthetic data for various robotic vision tasks. This virtual reality environment enables robotic vision researchers to generate realistic and visually plausible data with full ground truth for a wide variety of problems such as class and instance semantic segmentation, object detection, depth estimation, visual grasping, navigation, and more. 


Our motivation
==============

State-of-the-art deep learning architectures need large amounts of accurately annotated data for achieving a good performance. The lack of large-scale datasets which provide accurate ground truth (e.g. semantic segmentation, depth and normal maps, etc.) is mainly because data annotation is tedious and time-consuming. Because of that, photorealistic virtual reality environments are becoming increasingly popular and widely used to generate huge amounts of data with an accurate ground truth.

Taking advantage of this trend, we aim to provide a large-scale photorealistic dataset of indoor scenes where different robots can explore, manipulate, and interact with different objects. 


UnrealROX features
==================

1. Grasping system for robot manipulation is independent of the object geometry and orientation, allowing the robot to adopt different finger configurations.
2. Available for both Oculus Rift and HTC Vive Pro.
3. After recording, all scene, robot and camera information can be exported to a JSON file from Unreal Engine 4 (UE4)
4. With the JSON file and the unchanged scene where the sequence was recorded, you are able to generate the data you need. 
5. Scene cameras are fully configurable. You can place different cameras in the scene and also attach them to specific robot joints. 
6. This project is open-source. Anyone is welcome to contribute!



Our contribution
================

To the best of our knowledge, UnrealROX is the first photorealistic virtual reality environment based on Unreal Engine 4 and used for generating synthetic data for various robotic vision tasks.