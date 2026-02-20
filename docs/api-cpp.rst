C++ API
=======

The C++ interface is the most fully featured and is the natural choice when
writing native applications.  Include the header::

    #include <cwipc_util/api.h>

Link against ``cwipc_util`` (and the appropriate capturer modules such as
``cwipc_kinect``, ``cwipc_realsense2``, or ``cwipc_orbbec``).  Use ``cwipc::grabber`` classes for capture
and ``cwipc::viewer`` for rendering.

Example::

    #include <cwipc_util/api.h>
    int main() {
        cwipc::grabber g;
        g.open("--synthetic");
        while (auto p = g.grab()) {
            // process point cloud
        }
    }

Full API documentation is generated with Doxygen and can be found in the
``cwipc_util/doc`` directory or on the project's ReadTheDocs site once
published.
