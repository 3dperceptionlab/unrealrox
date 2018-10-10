***********
Basic usage
***********

.. _Interactive House: https://ue4arch.com/projects/interactive-house/
.. _Viennese Apartment: https://ue4arch.com/projects/viennese-apartment/
.. _Xoio Berlin Flat: https://www.unrealengine.com/marketplace/xoio-berlin-flat
.. _Hamburg House: https://ue4arch.com/projects/hamburg/

Now you have the UnrealROX project you will learn how to use it properly. First of all you need to get a photorrealistic scene or create your own. The scenes we used are the following:
* `Interactive House`_
* `Viennese Apartment`_
* `Xoio Berlin Flat`_
* `Hamburg House`_



Import and prepare your photorrealistic scene
#############################################

To import your scene to the UnrealROX project you need to do the following steps:

1. **Migrate your scene to UnrealROX**. On the one hand we have the UnrealROX project to which we want to import the scene from other UE4 project. Open your scene project and go to the Content Browser and localize *.umap* file of your scene, *right-click* and go to *Asset Actions->Migrate*. Now you need to navigate to the *Content* folder of UnrealROX project and select it as target for migrating your scene. 

2. **Put the robot in your scene**. The last step is to put the robot in your scene. From this point we will only work on the UnrealROX project. In the UE4 editor go to the Content Browser and navigate to *Mannequin->Meshes* selecting your pawn (e.g. *ROXMannequinPawn*) and drag it to the scene. You can scale your robot to match it with your scene scale.

3. **You are ready for the next step!**