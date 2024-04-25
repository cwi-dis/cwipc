# Setting up your cameras

Currently _cwipc_ supports Microsooft Kinect Azure and Intel RealSense D400 series cameras. It also supports pre-recorded footage of those cameras, as `.mkv` or `.bag` files. See below.

> Both types are fully supported on Windows. On Linux both types should be supported, but this has not been tested recently. On Mac only the Realsense cameras are supported, but there are major issues at the moment (bascially you have to run everything as `root`).

You should first _register_ your cameras. This creates a file `cameraconfig.json` that will have information on camera serial numbers, where each camera is located and where it is pointed, and how the captured images of the cameras overlap. This information is needed to be able to produce a consistent point cloud from your collection of cameras.

The preferred way to use your cameras is to put them on tripods, in _portait mode_, with all cameras having a clear view of the floor of your _origin_, the natural "central location" where your subject will be. But see below for situations where this is not possible.

You need to print the [origin marker](../cwipc_util/data/target-a4-aruco-0.pdf). If that link does not work: you can also find the origin marker in your installation directory, in `share/cwipc/registration/target-a4-aruco-0.pdf`, or online, at <https://github.com/cwi-dis/cwipc_util/blob/master/data/target-a4-aruco-0.pdf>.

Registering your cameras consists of a number of steps:

- Setup your hardware. 
- Use _cwipc\_register_ to find your cameras. This gives you an unaligned `cameraconfig.json`.
- Use _cwipc\_register_ to locate the origin marker in every camera, giving you a coarse alignment in `cameraconfig.json`.
- Use _cwipc\_register_ to do fine alignment.
- Manually edit `cameraconfig.json` to limit the point clouds to the subject (removing floor, walls, ceiling, etc).

Each of these steps is explained below.

## cwipc_register

The `cwipc_register` command line utility is the swiss army knife to help you setup your cameras, but it is rather clunky at the moment. An interactive GUI-based tool will come at some point in the future.

Use `cwipc_register --help` to see all the command line options it has. For now, we will explain the most important ones only:

- `cwipc_register` without any arguments will try to do all of the needed steps (but it is unlikely to succeed unless you know exactly what you are doing).
- The `--verbose` option will show verbose output, and it will also bring up windows to show you the result of every step. Close the window (or press `ESC` with the window active) to proceed with the next step.
- The `--rgb` will use the RGB and Depth images to do the coarse registration (in stead of the point clouds), if possible. This will give much better results. 
- The `--interactive` option will show you the point cloud currently captured. You can press `w` in the point cloud window to use this capture for your calibration step. If `--rgb` is also given you will be shown the captured RGB data, in a separate window. The `--rgb_cw` and `--rgb_ccw` options can be given to rotate the RGB images.

So, with all of these together, using `cwipc_register --rgb --interactive` may allow you to go through the whole procedure in one single step.

## Hardware setup

Ensure that all cameras are working (using _Realsense Viewer_ or _Azure Kinect Viewer_). If you have multiple cameras you are going to need _sync cables_ to ensure all the shutters fire simultanously for best results. See the camera documentation. You probably want to disable _auto-exposure_, _auto-whitebalance_ and all those.Set those to manual, and in such a way that the colors from all cameras are as close as possible.

You may need USB3 range extenders (also known as active cables) to be able to get to all of your cameras. Ensure these work within the camera viewer.

Put your origin marker on the floor and ensure all cameras can see it in RGB and Depth. The latter may be a bit difficult (because you can't see the marker in Depth). 

Have a person stand at the origin and ensure their head is not cut off. Adjust camera angles and such. Lock down the cameras, all of the adjustable screws and bolts and such on your tripods. And lock the tripods to the floor with gaffer tape.

## Finding your cameras

The first step is to use `cwipc_register --noregister` to create a `cameraconfig.json` file that simply contains the serial number of every camera. If there already is such a file in the current directory this step does nothing. Remove the `cameraconfig.json` file if you want to re-run.

Usually it will find what type of camera you have attached automatically. If this fails you can supply the `--kinect` or `--realsense` option to help it.

## Coarse registration

The easiest way to do coarse calibration is to put the origin marker on the floor and run `cwipc_register --rgb --nofine`. This will run a coarse calibration step for each camera in turn, _but only if the camera has not been coarse-calibrated before_. In other words, you can run this multiple times if some cameras were missed the previous time. But on the other hand if you had to move cameras you should remove `cameraconfig.json` and restart at the previous step.

For each camera, the RGB image is used to find the origin marker Aruco pattern. The Depth image is then used to find the distance and orientation of the Aruco marker from the camera. This information is then used to compute the `4*4` transformation matrix from camera coordinates to world coordinates, and this information is stored in `cameraconfig.json`.

If the Aruco marker cannot be found automatically you can also use a manual procedure, by **not** supplying the `--rgb` argument. You will then be provided with a point cloud viewer window where you have to manually select the corners of the marker (using shift-click with the mouse) **in the right order**.

After this step you have a complete registration. You can run `cwipc_view` to see your point cloud. It should be approximately correct, but in the areas that are seen by multiple cameras you will see the the alignment is not perfect.

## Fine registration

## Limiting your point clouds

## Special cases

### Registering pre-recorded footage

### Registering large spaces

### Registering head-and-shoulders shots



