# Setting up your cameras

Currently _cwipc_ supports Microsooft Kinect Azure and Intel RealSense D400 series cameras. It also supports pre-recorded footage of those cameras, as `.mkv` or `.bag` files. See below.

> Both types are fully supported on Windows. On Linux both types should be supported, but this has not been tested recently. On Mac only the Realsense cameras are supported, but there are major issues at the moment (bascially you have to run everything as `root`). Realsense recordings are supported on Mac.


The preferred way to use your cameras is to put them on tripods, in _portait mode_, with all cameras having a clear view of the floor of your _origin_, the natural "central location" where your subject will be. But see below for situations where this is not possible.

> For testing it may be possible that you don't have to do any registration at all, see the _Head and Shoulders_ section below.

Here is a picture of a four-camera setup:

![Pyisical setup of four cameras](camera-setup.jpg)



You need to print the [origin Aruco marker](../cwipc_util/data/target-a4-aruco-0.pdf), which you see in the center of the picture above. If that link does not work: you can also find the origin marker in your installation directory, in `share/cwipc/registration/target-a4-aruco-0.pdf`, or online, at <https://github.com/cwi-dis/cwipc_util/blob/master/data/target-a4-aruco-0.pdf>, and use the `...` menu and _Download Raw File Content_ from there.

Registering your cameras consists of a number of steps:

- Setup your hardware.
- Find the cameras attached to your system.
- Setup correct camera views, exposure, etc.
- Do a coarse registration by finding the Aruco marker image. This creates a 4x4 transformation matrix for each camera which will ensure that all cameras use approximately the same coordinate system.
- Do a fine registration by capturing a human and slightly adjusting the transformation matrices so we get the best possible overlap between the captures.

Normally the following command will take you through each of these steps (except for setting up the hardware) and tell you what to do:

```
cwipc_register --guided
```

But sometimes it may be needed to have more control over the process, or repeat a step, or something else. For that it is possible to use arguments to do each of the steps separately.

## cwipc_register

The `cwipc_register` command line utility is the swiss army knife to help you setup your cameras, but it is rather clunky at the moment. An interactive GUI-based tool will come at some point in the future.

Use `cwipc_register --help` to see all the command line options it has. For now, we will explain the most important ones only:

- `cwipc_register` without any arguments will try to do all of the needed steps (but it is unlikely to succeed unless you know exactly what you are doing).
- The `--verbose` option will show verbose output, and it will also bring up windows to show you the result of every step. Close the window (or press `ESC` with the window active) to proceed with the next step.
- The `--rgb` will use the RGB and Depth images to do the coarse registration (in stead of the point clouds), if possible. This will give much better results. 
- The `--interactive` option will show you the point cloud currently captured. You can press `w` in the point cloud window to use this capture for your calibration step. If `--rgb` is also given you will be shown the captured RGB data, in a separate window. The `--rgb_cw` and `--rgb_ccw` options can be given to rotate the RGB images.
	- In `--interactive` mode the `cwipc_register` point cloud window works similar to the `cwipc_view` window. So you can use left-mouse-drag to pan around the point cloud, right-mouse-drag to move up and down, scrollwheel to zoom. `?` will print some limited help on `stdout`.

So, with all of these together, using `cwipc_register --rgb --interactive` may allow you to go through the whole procedure in one single step.

## Hardware setup

Ensure that all cameras are working (using _Realsense Viewer_ or _Azure Kinect Viewer_). If you have multiple cameras you are going to need _sync cables_ to ensure all the shutters fire simultanously for best results. See the camera documentation. You probably want to disable _auto-exposure_, _auto-whitebalance_ and all those. Set those to manual, and in such a way that the colors from all cameras are as close as possible.

You may need USB3 range extenders (also known as active cables) to be able to get to all of your cameras. Ensure these work within the camera viewer.

### Checking your camera position and orientation

> Usually `cwipc_register --guided` will take you through this process just as easily. This section left here for reference.

Put your origin marker on the floor and ensure all cameras can see it in RGB and Depth. The latter may be a bit difficult (because you can't see the marker in Depth). 

Here is an example of what you should see in Kinect Viewer (or similar in Realsense Viewer):

![Screenshot of RGB and D capture](rgbd-capture.jpg)

Have a person stand at the origin and ensure their head is not cut off. Adjust camera angles and such. Lock down the cameras, all of the adjustable screws and bolts and such on your tripods. And lock the tripods to the floor with gaffer tape.

## Finding your cameras

> Usually `cwipc_register --guided` will take you through this process just as easily. This section left here for reference.

The first step is to use `cwipc_register --noregister` to create a `cameraconfig.json` file that simply contains the serial number of every camera. If there already is such a file in the current directory this step does nothing. Remove the `cameraconfig.json` file if you want to re-run.

Usually it will find what type of camera you have attached automatically. If this fails you can supply the `--kinect` or `--realsense` option to help it.

## Coarse registration

> Usually `cwipc_register --guided` will take you through this process just as easily. This section left here for reference.

The easiest way to do coarse calibration is to put the origin marker on the floor (the picture above gives you an idea of where you should place your origin marker) and run `cwipc_register --rgb --nofine`. 

This will run a coarse calibration step for each camera in turn, _but only if the camera has not been coarse-calibrated before_. In other words, you can run this multiple times if some cameras were missed the previous time. But on the other hand if you had to move cameras you should remove `cameraconfig.json` and restart at the previous step.

For each camera, the RGB image is used to find the origin marker Aruco pattern. The Depth image is then used to find the distance and orientation of the Aruco marker from the camera. This information is then used to compute the `4*4` transformation matrix from camera coordinates to world coordinates, and this information is stored in `cameraconfig.json`.

If the Aruco marker cannot be found automatically you can also use a manual procedure, by **not** supplying the `--rgb` argument. You will then be provided with a point cloud viewer window where you have to manually select the corners of the marker (using shift-click with the mouse) **in the right order**.

After this step you have a complete registration. You can run `cwipc_view` to see your point cloud. It should be approximately correct, but in the areas that are seen by multiple cameras you will see the the alignment is not perfect.

![Captured point cloud from all cameras after coarse registration](coarse-pointcloud.jpg)

## Limiting your point clouds

> This section is important, even when using `cwipc_register --guided` 

At this point, if you view your point cloud with `cwipc_view` you will see that it contains all the walls, floor, ceiling, furniture, etc. All cameras should be somewhat aligned by the course calibration.

> Currently you have to fix this by manually editing `cameraconfig.json`. It should be possible to edit `cameraconfig.json` while `cwipc_view` is running, and then typing `c` to reload `cameraconfig.json`. But this does not always work, you may have to stop and restart `cwipc_view` to see the result of your edits.

### Near and far points

The first thing to edit is `threshold_near` and `threshold_far`. These are the near and far point for all depth cameras (in meters). Adjust these to get rid of most of the walls, while keeping the whole target area visible. Looking at the floor is a good way to determine how you are doing.

### Radius

There is another parameter you can play with: `radius_filter` applies a cylindrical filter around the origin.

### Remove ceiling

Next adjust `height_min` and `height_max` to get rid of the ceiling, **but keep the floor for now**. Both have to be non-zero otherwise the filter will not be applied (but `height_min` can be less than zero if you want to keep the floor visible).


## Fine registration

> Usually `cwipc_register --guided` will take you through this process just as easily. This section left here for reference.

If you have only a single RGBD camera there is no point in doing fine calibration, but if you have multiple cameras it will slightly adjust the registration of the cameras to try and get maximum overlap of the point clouds.

Have a person stand at the origin.

> **Note:** at the moment, the pose of you subject is important. Best results are obtained by having the person look in the direction between the first camera and one of the adjacent cameras. Arms should be slightly spread, or maybe angled forward at the elbows.

Run `cwipc_register`. If there is already a complete coarse calibration for all cameras this will automatically do a fine calibration. If you are using `cwipc_register --interactive` type a `w` to capture a point cloud and start the registration.

The algorithm will iterate over the cameras, making slight adjustments to the alignment. When it cannot improve the results any more it stops and saves `cameraconfig.json`.

Check the results with `cwipc_view`.

> The algorithm is not perfect, and it can some times get into a local minimum. You will see this as a disjunct point cloud, often with cameras pairwise aligned and those pairs disaligned.
> 
> The workaround is to try fine alignment again, with the subject standing in a different pose.

### Remove floor and ceiling

Next adjust `height_min` and `height_max` to get rid of both floor and ceiling. Both have to be non-zero otherwise the filter will not be applied.

### Color matching

You probably want to play with the various exposure parameters such as `color_whitebalance` and `color_exposure_time` to get the best color fidelity, but you really have to experiment here.

We are working on partially automating this process, but that is not done yet.

Depth exposure can be left on auto (if the camera supports it) but color should be manual, so that it is the same for all cameras.

### Final results

After all the steps have been done you should be able to get point clouds like the one below. 

![Calibrated point cloud captured with cwipc_view](calibrated.jpg)

## Special cases

Here are some odds and ends that did not fit anywhere else.

### Registering head-and-shoulders shots

There is a way to register a single camera without any makers at all. If your camera is in landscape orientation, at approximately `1.20m` height, and approximately `1m` away from a seated subject.

Incidentally, this is exactly what happens if you put the camera on your desk, on a small tripod, next to your monitor.


If you now run `cwipc_register --tabletop` you will get a `cameraconfig.json` that will capture a head and shoulders shot of you.

Here is a picture of the setup and the resulting point cloud in `cwipc_view`:

![Tabletop Setup](tabletop-setup.jpg)

This is useful for developer testing, but it may also be usable for virtual meetings, etc.

### Registering pre-recorded footage

If you have pre-recorded footage (`.mkv` files for Kinect, `.bag` files for realsense) and you have captured some sort of a marker (either the Aruco origin marker, or the older four-color-corner marker) you can create a `cameraconfig.json` for this recording:

- Put all your files in a single directory, lets say `new-recording`.
- In that directory, run `cwipc_register --noregister new-recording`. This will create the initial `cameraconfig.json` inside that directory, with references to all the recording files.
  - **NOTE**: if you still have access to the original `cameraconfig.json` that was used to capture the recording it is better to manually copy that file here and edit it to become a config for a recording. Details will be added here at some point.
- In that directory, run `cwipc_register --guided`. But note that you cannot change any of the hardware parameters, such as `fps` or capture width and height.


### Registering large spaces

> This section is not for the faint of heart.

If you have a large space, or if not all of your cameras can see the origin, you may still be able to do a registration using the _Auxiliary Aruco Markers_.

In the place where you found the `target-a4-aruco-0.pdf` there are 5 extra markers. These can be used as a "bread crumb trail": put the auxiliary markers on the floor in random places, but ensure that every marker can be seen by at least two cameras, and that there is a bread crumb trail from every marker back to the origin marker.

You may want to print the markers at 140% (i.e. on A3 paper).

`cwipc_register` will first do the coarse alignment only of the cameras that can see the origin marker. But after that it will have recorded the position of some of the auxiliary markers. It will then do a second pass, in which it will align the cameras that could see that second marker but not the origin marker. This is then repeated until all cameras are registered.

### Registering without Aruco markers

> This section is not for the faint of heart.

If you need to register cameras but you cannot print the Aruco markers you can do a manual coarse registration if you have a piece of paper that is approximately A4-sized. And if you don't even have that you can still follow this procedure but you will have to guess how big a piece of A4 paper would be, approximately, and simply select points on the floor where the A4 paper would have been.

Place it on the ground in the origin position and run `cwipc_register --no_aruco --nofine`.

You will be presented with a 3D view of the capture. You can use the mouse and scrollwheel to change your viewpoint.

You must now select 4 points (with shift-left-click) in a specific order:

1. Near-left point of the A4 (from the camera view point),
2. Near-right point,
3. Far-right point,
4. Far-left point.

Inspect the result of this coarse calibration with `cwipc_view`, and make double-sure that the blue, red and green axes are pointing in the right direction (see image above).


