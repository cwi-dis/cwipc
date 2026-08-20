# cwipc Python examples

Here are some very simple examples of how to use the `cwipc` modules from Python. They are mainly modeled after the C and C++ programs from the `apps` directories in the source tree.

The `cwipc` module should be installed already. If not you can run the command

```
% cwipc_pymodules_install.sh
```

or (on Windows)

```
C:/> cwipc_pymodules_install.bat
```

The Python package is documented and type-hinted, so if you use an IDE like `vscode` you should get popup hints as you type.

> If you have used cwipc before v8.0 you may remember having to call `free()` yourself. This is no longer the case.

The examples here are pretty minimal: no reasonable command line arguments, and little or no respect for timing.

### generate.py

Create a number of pointclouds with the synthetic generator and store them as ply-files.

### ply2dump.py

Convert a pointcloud ply-file to a `.cwipcdump` file (a more efficient storage format that mimicks the in-core format of point cloud objects and can be read and written fast)

### dump2ply.py

Convert a `.cwipcdump` file back to a ply-file.

### downsample.py

Read a point cloud from a ply-file, voxelize it, store it in a new ply-file.

### viewsynthetic.py

Create a stream of pointclouds with the synthetic generator and show them in a window. You can use the mouse to change your view.

### viewcamera.py

Capture pointclouds from an attached camera and show them in a window. You probably need to configure the camera beforehand with `cwipc_register`.

### record.py

Record a number of pointclouds (captured from the camera) to a directory as cwipcdump files.

### viewfile.py

View a pointcloud from a ply-file or a `.cwipcdump` file.

### viewrecording.py

View a sequence of pointclouds from a directory. The viewing will loop, starting over when done, until you close the window.

A pointcloud sequence directory can be created with `generate.py` or `record.py`.

### compressfile.py

Compresses a point cloud file using the MPEG Anchor point cloud codec, producing a `.cwicpc` compressed pointcloud file.

### decompressfile.py

Decompresses a `.cwicpc` file to a `.ply` or `.cwipcdump` point cloud file.