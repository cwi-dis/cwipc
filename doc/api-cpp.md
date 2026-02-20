# C++ API

The C++ interface is the most fully featured and is the natural choice when
writing native applications.  Include the header:

```cpp
#include <cwipc_util.hpp>
```

Link against `cwipc_util` (and the appropriate capturer modules such as
`cwipc_kinect` or `cwipc_realsense2`).  Use `cwipc::grabber` classes for capture
and `cwipc::viewer` for rendering.

Example:

```cpp
#include <cwipc_util.hpp>
int main() {
    cwipc::grabber g;
    g.open("--synthetic");
    while (auto p = g.grab()) {
        // process point cloud
    }
}
```

Full API documentation is generated with Doxygen and can be found in the
`cwipc_util/doc` directory or on the project's ReadTheDocs site once
published.

[Back to Programming Interfaces](api-overview.md)
