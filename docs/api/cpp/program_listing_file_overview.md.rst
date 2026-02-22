
.. _program_listing_file_overview.md:

Program Listing for File overview.md
====================================

|exhale_lsh| :ref:`Return to documentation for file <file_overview.md>` (``overview.md``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: markdown

   # cwipc C and C++ API
   
   ## cwipc_pointcloud
   
   The `cwipc_pointcloud` object is intended as an abstract object representing a pointcloud.
   
   It is implemented in the `cwipc_util` library, together with some auxiliary obects to obtain
   point cloud objects, compress them, etc.
   
   The library can be used from C and C++. In the latter case it will expose a virtual object API, in the former case an opaque object is passed as the first argument to many functions. Interfaces for Python and C# are also available, and mimick the C++ interface.
   
   In case the library is used from C++ it can also export a PCL (Point Cloud Library) API, which allows access to the underlying PCL implementations of the pointclouds.
   
   ## cwipc_source
   
   The `cwipc_source` and `cwipc_activesource` objects are interfaces that produce `cwipc_pointcloud` objects. Think: cameras, file readers, decompressors.
   
   ## cwipc_sink
   
   The `cwipc_sink` objects are interfaces that consume `cwipc_pointcloud` objects. Think: display windows, compressors, file writers.
   
   ## Top-level global functions
   
   There are a few global functions to control overall functionality of cwipc:
   
   - `cwipc_get_version` to get the current version, as a string.
   - `cwipc_log_configure` to setup logging.
   - `cwipc_dangling_allocations` for debugging memory management (your calling of `free()`)
   
   ## Creating point clouds
   
   Point clouds are created through a `cwipc_source` or using the functions `cwipc_read`, `cwipc_read_debugdump`, `cwipc_from_points`, `cwipc_from_packet`.
   
   ## Saving point clouds
   
   Point clouds can be written to file with `cwipc_write`, `cwipc_write_ext` and `cwipc_write_debugdump`.
   
   ## Creating point cloud sources
   
   Instances of `cwipc_activesource` are created with `cwipc_synthetic`, `cwipc_capturer` and `cwipc_proxy`.
   
   ## cwipc_pointcloud C++ interface
   
   To use abstract `cwipc_pointcloud` pointclouds, transfer them to other libraries (modules, languages) and to get access to the external representation of the points:
   
   ```
   #include "cwipc_util/api.h"
   ```
   
   
   Here are the main C++ methods (with all the `virtual` and `= 0;` and such removed for readability):
   
   ```
   class cwipc {
           void free();
           uint32_t timestamp();
           size_t get_uncompressed_size();
           void copy_uncompressed(struct cwipc_point *, size_t size);
           pcl_pointcloud *access_pcl_pointcloud();
   };
   ```
   
   The header file is fully documented in Doxygen, but here is the quick breakdown:
   
   - Call `free()` when you no longer need to access this pointcloud. Failure to call `free()` may lead to memory leaks. Accessing PCL pointcloud data after calling `free()` may lead to crashes.
   - Call `timestamp()` to get a (microsecond) timestamp indicating when the pointcloud was captured.
   - Call `get_uncompressed_size()` to see how many bytes the pointcloud data needs. Allocate a buffer of the correct size. Then call `copy_uncompressed()` to copy the point data into the buffer.
   
   The `cwipc_point` structure is the external representation of a point (and a pointcloud external representation is simly an array of points). Here is the point structure:
   
   ```
   struct cwipc_point {
       float x;
       float y;
       float z;
       uint8_t r;
       uint8_t g;
       uint8_t b;
   };
   ```
   
   ## cwipc C++ PCL interface
   
   Include the PCL-compatible header before you include
   the general *api.h* header:
   
   ```
   #include "cwipc_util/api_pcl.h"
   #include "cwipc_util/api.h"
   ```
   
   Again, the API is documented using Doxygen. But in short:
   
   - `access_pcl_pointcloud()` returns a reference to the underlying PCL pointcloud. This will remain valid until `free()` is called.
   
   ## cwipc C interface
   
   The C interface is contained in the same header as the C++ interface:
   
   ```
   #include "cwipc_util/api.h"
   ```
   
   The C functions are have the same names as the C++ methods above, but with `cwipc_pointcloud_` prepended.
   
   These functions take a `cwipc_pointcloud *` first argument, and the rest of their arguments are the same as for their C++ counterparts.
   
   ## Utility function interface
   
   Again fully documented in Doxygen, but there are functions to read and write PLY files, convert external representation to cwipc pointclouds and convert PCL pointclouds to cwipc pointclouds.
   
   ## Historic reasons for design of API for pointclouds
   
   > NOTE: the following section is no longer factually correct, but it is the original design document
   > written around 2019. We keep it around because it provides insight into the design ideas for cwipc.
   
   We need APIs for capturing compressing and decompressing pointclouds, and while we're at it we might as well add APIs for reading and writing them to files.
   
   There are two libraries involved:
   
   - [CWI Codec](https://github.com/cwi-dis/cwi_codec_lib)
   - [Realsense Capturer](https://github.com/cwi-dis/VRTogether-capture)
   
   There are currently two main users of the API:
   
   - pcl2dash (which isn't in github at the moment)
   - Unity Renderer (in i2cat repo, to be provided)
   
   The API needs to be accessible from C++ (for pcl2dash) and C# (for Unity). For the latter we need a C API that is bridgeable to C#, i.e. the _unmanaged pointers_ (C# terminology) need to be convertible to managed data structures with as little copying as possible. It would be good if the C API has such a form that
   
   -  it would also be usable from C directly (not only C#), 
   -  it would be usable on other platforms than Windows,
   -  it would be usable in other languages (Python comes to mind).
   
   The pointclouds are stored internally (in the codec and capturer) as [libpcl](http://pointclouds.org) data structures. I think these are in turn based on [Boost](https://www.boost.org) data structures.
   
   It is a requirement that the API can be accessed trough a dynamic library, possibly loaded at runtime (requirement for C#).
   
   It is a requirement that the DLL or program that has allocated storage is also responsible for deallocating it (to forestall C++ runtime system issues on Windows).
   
   It is a requirement that the PCL datastructures (uncompressed point clouds) can be transferred between the capturer and the encoder without copying.
   
   It is a requirement that the uncompressed pointclouds can also be made available in a non-PCL non-boost format (for the Unity renderer).
   
   It is a requirement that _pcl2dash_ does not have to include PCL or Boost or other headers (it only has to transfer the pointcloud data structures between the capturer and the codec).
   
   ## Header files, stubs
   
   There are a number of distinct header files for different types of use:
   
   - a combined C/C++ header file `cwipc.h` which does not require including PCL or Boost headers, and which allows copying the pointcloud data (see below). The header uses the usual tricks for hiding the C++ code from C and adding the `extern "C"` bits where needed. 
   - another header file `cwipc_ex.h` (or `.hpp`?) which does include the PCL and Boost headers and is used inside the capture and codec libraries (and possibly in other programs that need to access the PCL representations). This file includes `cwipc.h`.
   - Separate header files for the capturer and the codec APIs.
   
   > Discussion point: It feels elegant to have to C# bridge (the code that currently lives in Unity, in <https://github.com/cwi-dis/VRTogether-PointCloud-Rendering/tree/master/Unity/Assets/Scripts/CWI>) be part of our API. But I don't know how well that integrates with Unity (can unity refer to scripts that don't live inside their directory structure? Or should we simply say that the scripts need to be copied verbatim into the directory structure, just as the DLLs are also copied at the moment?).
   
   ## Data structures
   
   Uncompressed pointclouds produced by the capturer or decoder are represented by an opaque datastructure `opaque_pointcloud`. In C++ this is a class in the `cwipc` namespace with methods to access it. In C it is a `struct cwipc_opaque_pointcloud` that is passed as the first parameter to the accessor functions.
   
   Here are the C++ methods (with all the `virtual` and `= 0;` and such removed for readability):
   
   ```
   namespace cwipc {
   struct pointcloud;
   class pcl_pointcloud;
   
   class opaque_pointcloud {
           void free();
           uint32_t timestamp();
           size_t get_uncompressed_size();
           void copy_uncompressed(struct pointcloud *, size_t size);
           pcl_pointcloud *access_pcl_pointcloud();
   };
   };
   ```
   
   > Discussion point: what is the correct type for the timestamp?
   
   > Discussion point: if a pointcloud has been captured by multiple cameras and we need the multiple angles sometimes separately, sometimes together, is this a different datastructure (`opaque_multi_pointcloud`) with different accessors, or do we add `partial` variants to the get/copy/access methods to allow getting only partial pointclouds?
    
   Note that the opaque pointcloud object is the owner of the the underlying PCL pointcloud, and calling its `free()` method will invalidate the PCL pointcloud reference returned by `access_pcl_pointcloud()`. In case a consumer of the pointcloud wants to access the individual points it first gets the size in bytes required (with `get_uncompressed_size()`, then allocates a buffer of that size, then passes that buffer to `copy_uncompressed()`. It is suggested that the main program (i.e. the caller of the capturer or decoder method that returned this `opaque_pointcloud`) is responsible of calling free (so not having the compressor call free all by itself after being done with the data).
   
   In case the C api is used the caller has to ensure that it calls the `cwipc_free()` function from the same DLL that originally allocated the opaque pointcloud.
   
   Here is the C API:
   
   ```
   struct cwipc_opaque_pointcloud;
   struct cwipc_pointcloud;
   
   void cwipc_free(struct cwipc_opaque_pointcloud *);
   uint32_t cwipc_timestamp(struct cwipc_opaque_pointcloud *);
   size_t cwipc_get_uncompressed_size(struct cwipc_opaque_pointcloud *);
   void cwipc_copy_uncompressed(struct cwipc_opaque_pointcloud *, struct pointcloud *, size_t size);
   
   ```
   
   Uncompressed pointclouds usable by consumers are represented as simple structs:
   
   ```
   struct cwpipc_point {
           int32_t x, y, z;
           uint8_t r, g, b;
   };
   
   struct cwipc_pointcloud {
           uint32_t npoints;
           struct cwipc_point points[1];
   };
   ```
   
   > Discussion point: should a point contain an indication of which cameras contributed to it? 
   
   > Discussion point: what are the types for x/y/z and r/g/b?
   
   > Discussion point: are compressed pointclouds represented simply as a byte string (i.e. a `void *` and a `size_t`)? Or should we do a similar thing which `opaque_compressed_pointcloud` to ensure alloc/free is handled by the correct library? Or should we do the `std::stringstream` thing currently used, even though that means there's an issue with C use (as opposed to C++) and we have yet another memory copy...
   
   > To do: the exact structure of the `cwipc` namespace and the `cwpic_` prefix for C use needs to be worked out.
