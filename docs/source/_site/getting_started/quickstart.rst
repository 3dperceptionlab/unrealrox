**********
Quickstart
**********

.. _pivot tool: https://www.unrealengine.com/marketplace/pivot-tool


This is a short tutorial with the main purpose of teaching you to use UnrealROX in your scene. In order to get started you need to perform the following steps:

1. First of all you need a UE4 scene as photorealistic as possible to impress your boss! 

Do this for each scene you want to use
######################################

2. Run your scene and check that all object pivots are placed on the geometrical center of its meshes for configure X and Y axis (Note: Z axis should be 0 for a better interaction with some objects). This is important to track all the objects correctly during the recording and playback steps. However, this is a tedious task thus realistic scenes have lots of objects. Due to, we use the following `pivot tool`_ which works like a charm.
3. Choose the object you want to interact with from the World Outlier (by default placed on the right side in the UE4 editor) and do the following:
	1. Go to Transform->Mobility and set the object to Movable. Almost all the objects are static by default.
	2. Go to Physics and check Simulate Physics option.
	3. Go to Collision and check Generate Overlap Events option. This is a must for the grasping system in order to grab an object correctly. If this option is disabled, grasping wouldn't work.
	4. You also need to check object geometry to achieve a visually plausible grasping. Right-click on the object and Edit. In the Object Editor you need to visualize the simple and complex collision meshes and visually check its accuracy. If object geometry is complex and the collision mesh is rough, you maybe can improve this by auto generating convex collision (Collision menubar->Auto Convex Collision) with maximum hull verts and accuracy. You also need to set for complex object geometries the Collision Complexity to "Use simple collision as complex". This is to achieve a more realistic grasping, however, physics simulation is much more complicated and when interacting with two or more objects you may notice an unstable behavior.
4. Now you need to build project lighting! The steps above are very important, especially the step of setting the objects as movable. You need to set lighting configuration according to your PC specs. For a fast and feasible lighting generation we recommend the following configuration you can do in the World Settings-> Lightmass. See the following figure.


.. image:: /_static/lightmass.jpg
  :width: 400
  :alt: Lightmass configuration