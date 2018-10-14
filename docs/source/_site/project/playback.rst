.. _playback:

********
Playback
********

With this final step you will generate the ground truth (e.g. semantic segmentation and depth maps, normal maps, stereo pairs, and also instance segmentation, etc.) of your recorded sequence. First of all you need to convert your recordings stored as *.txt* files in the *RecordedSequences* folder to *.json* files compatible with the playback process. 


Convert recorded sequences to JSON
##################################

Go to *ROXTracker* located in the *World Outliner* panel in the UE4 editor and search for *JSON Management* (*see Figure 1*).

.. figure:: /_static/playback_0.png
    :scale: 100 %
    :align: center
    :alt: General parameters for recording and playback steps.
    :figclass: align-center

    Figure 1. Convert recorded sequences to JSON.

Set the *Input Scene TXT File Name* and also the *Output Scene Json File Name* and click on *Generate Sequence Json*. Json files will be stored in the *RecordedSequences* folder.



Configure playback process
##########################

Go to *ROXTracker* located in the *World Outliner* panel in the UE4 editor and search for *Playback* (*see Figure 2*). 

.. figure:: /_static/playback_1.png
    :scale: 100 %
    :align: center
    :alt: Configurable parameters for playback process.
    :figclass: align-center

    Figure 2. Configurable parameters for the playback process.


Proceed with the following steps:

- **Specify JSON files**: add JSON file names to the *Json File Names* array.

- **Start from a given frame**: if playback process was accidentally interrupted you can resume the process indicating the latest generated frame (Default: 0).

- **Select the desired data to generate**: check the desired options you want to generate. You can also choose RGB data format.

- **Path**: choose where to save the data. *Screenshots Save Directory* and *Screenshots Folder* parameters.

- **Data resolution**: choose generated data resolution (Default: 1920x1080).



Run playback process
####################

In order to proceed with the playback process, you will need to uncheck *Record mode* from the general *ROXTracker* configuration (*see Figure 1 from* :ref:`recording` *section*). Run project in the *Selected Viewport* mode [#f1]_. All the data will be saved by default on GeneratedSequences folder located in the root of UnrealROX project.



.. rubric: Footnotes

.. [#f1] If your main purpose is to generate data and you run the project in *VR Preview* mode, UnrealROX wouldn't work properly.