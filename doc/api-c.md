# C API

For applications requiring a pure C interface or when interfacing from other
languages via FFI, the C API exposes the same functionality as the C++
bindings.

Include the header:

```c
#include <cwipc.h>
```

Link against the `cwipc_util` library.  Memory management is manual; callers are
responsible for releasing handles with `cwipc_release()`.

```c
#include <cwipc.h>
int main(void) {
    cwipc_grabber g = cwipc_grabber_create();
    cwipc_grabber_open(g, "--synthetic");
    cwipc_frame f;
    while (cwipc_grabber_grab(g, &f) == CWIPC_OK) {
        // use f.points etc.
        cwipc_release(f.ptr);
    }
    cwipc_grabber_destroy(g);
    return 0;
}
```

See the header comments and generated Doxygen pages for full details.

[Back to Programming Interfaces](api-overview.md)
