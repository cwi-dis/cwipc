Introduction
============

**cwipc** is a cross-platform suite for working with *dynamic point clouds*.
It provides:

* A language‑neutral core representation (``cwipc`` objects) with bindings for
  C, C++, Python and C#.
* Capturers for Azure Kinect, Intel RealSense, and Orbbec Femto depth cameras, supporting
  multi‑camera setups with registration and synchronization.
* Compression codecs (``cwipc_codec``) for real‑time transmission.
* A collection of command‑line utilities (``cwipc view``, ``cwipc grab``,
  ``cwipc forward``, etc.) for capture, playback, streaming and analysis.

Use cases
---------

1. **Real‑time capture and streaming** – build applications that acquire
   point clouds from hardware, compress them and send them over the network
   (for example live telepresence or social XR).
2. **Offline processing** – record camera streams, replay them, convert
   between formats, and apply filters/transformations programmatically.
3. **Research and analysis** – use Python/Jupyter notebooks with NumPy,
   PyOpen3D or other libraries to study 3D data, machine learning models, or
   dataset creation.
4. **Unity integration** – develop immersive experiences using the
   ``cwipc_unity`` package to ingest and render point clouds from C#.
5. **Cross‑language pipelines** – the opaque core object can be passed
   between components written in different languages with minimal copying.

The library is suitable for prototyping, experimentation, and production
software on Windows, macOS, Linux and Android devices.
