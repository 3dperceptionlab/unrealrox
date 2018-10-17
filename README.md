# UnrealROX

Data-driven algorithms have surpassed traditional techniques in almost every aspect in robotic vision problems. Such algorithms need vast amounts of quality data to be able to work properly after their training process. Gathering and annotating that sheer amount of data in the real world is a time-consuming and error-prone task. Those problems limit scale and quality. Synthetic data generation has become increasingly popular since it is faster to generate and automatic to annotate. However, most of the current datasets and environments lack realism, interactions, and details from the real world. UnrealROX is an environment built over Unreal Engine 4 which aims to reduce that reality gap by leveraging hyperrealistic indoor scenes that are explored by robot agents which also interact with objects in a visually realistic manner in that simulated world. Photorealistic scenes and robots are rendered by Unreal Engine into a virtual reality headset which captures gaze so that a human operator can move the robot and use controllers for the robotic hands; scene information is dumped on a per-frame basis so that it can be reproduced offline to generate raw data and ground truth annotations. This virtual reality environment enables robotic vision researchers to generate realistic and visually plausible data with full ground truth for a wide variety of problems such as class and instance semantic segmentation, object detection, depth estimation, visual grasping, and navigation.

## Documentation

Visit the [ReadTheDocs](https://unrealrox.readthedocs.io/en/latest/) website for instructions on how to setup the environment and detailed usage tutorials.

## Paper

Pablo Martinez-Gonzalez, Sergiu Oprea, Alberto Garcia-Garcia, Alvaro Jover-Alvarez, Sergio Orts-Escolano, Jose Garcia-Rodriguez
[UnrealROX: An eXtremely Photorealistic Virtual Reality Environment for Robotics Simulations and Synthetic Data Generation](https://arxiv.org/abs/1810.06936)
Submitted to Virtual Reality journal.


If you use UnrealROX, please cite:

```
@article{Unrealrox2018,,
  title={{UnrealROX}: An eXtremely Photorealistic Virtual Reality Environment for Robotics Simulations and Synthetic Data Generation},
  author={Martinez-Gonzalez, Pablo and Oprea, Sergiu and Garcia-Garcia, Alberto and Jover-Alvarez, Alvaro and Orts-Escolano, Sergio and Garcia-Rodriguez, Jose},
  journal={ArXiv e-prints},
  eprint = {1810.06936},
  year={2018}
}
```

## License

This code is released under the MIT License.

## Contact

Any criticism and improvements is welcome using the issue system from this repository. For other questions, contact the corresponding authors:

- Pablo Martinez-Gonzalez: pmartinez@dtic.ua.es
- Sergiu Oprea: soprea@dtic.ua.es
- Alberto Garcia-Garcia: agarcia@dtic.ua.es
