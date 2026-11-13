Point cloud files
=================

``cwipc`` reads and writes single point clouds as PLY files (:cpp:func:`cwipc_read`,
:cpp:func:`cwipc_write`, :cpp:func:`cwipc_write_ext`), and also has its own
``.cwipcdump`` format. Since version 8.3.0 the PLY reading and writing is cwipc's own code
rather than PCL's; the decisions behind it are recorded in
`cwipc#66 <https://github.com/cwi-dis/cwipc/issues/66>`_.

PLY files written by cwipc
--------------------------

:cpp:func:`cwipc_write` writes ASCII PLY; :cpp:func:`cwipc_write_ext` with
``CWIPC_FLAG_BINARY`` writes binary (little-endian) PLY. Both use this header::

    ply
    format ascii 1.0                     (or: format binary_little_endian 1.0)
    comment written by cwipc
    element vertex 378871
    property float x
    property float y
    property float z
    property uchar red
    property uchar green
    property uchar blue
    property uchar tile
    element cwipc_metadata 1
    property double timestamp
    property float cellsize
    end_header

* The vertex properties are in the order of ``struct cwipc_point``. The fourth byte is the
  tile number (which can also be read as a mask of contributing cameras).
* ``cwipc_metadata`` holds the point cloud's timestamp, **in seconds** (cwipc's own
  timestamps are milliseconds; as a ``double`` they convert back exactly), and its cellsize.
  Other tools ignore this element.
* In binary files the header is padded to a multiple of 16 bytes with a
  ``comment padding xxx...`` line, so the vertex data that follows has exactly the
  ``struct cwipc_point`` layout (16 bytes per point) and starts at an aligned offset.
* ASCII files always have LF line endings, and numbers are written in the shortest form
  that reads back to exactly the same value, independent of the locale.

Open3D, MeshLab and CloudCompare read these files, with their colours. They don't use the
``tile`` property.

**Compatibility:** cwipc before 8.3.0 wrote the fourth byte as ``alpha``, which made
MeshLab show washed-out colours. cwipc before 8.3.0 reads files written by 8.3.0 and
later with all tile numbers 0.

PLY files cwipc can read
------------------------

:cpp:func:`cwipc_read` reads PLY files that cwipc writes, and plain point cloud PLY files
from other tools:

* ASCII (LF or CRLF line endings) or binary, in either byte order.
* A ``vertex`` element with ``x``, ``y`` and ``z``, of any numeric type. Without them the
  file is refused.
* Optionally ``red``, ``green`` and ``blue`` of type ``uchar``, and the fourth byte as a
  ``uchar`` property ``tile``, or ``alpha`` (files written by cwipc before 8.3.0).
* Everything else is skipped: other properties (such as normals) and other elements
  (such as ``face``, or the ``camera`` element PCL used to write).
* Colours that cwipc can't use give a warning (through the log callback registered from
  Python or C#, or on stderr; not as an error) and black points: ``red``/``green``/``blue`` of a type other than
  ``uchar``, and Gaussian splat colours (``f_dc_0`` and further). Other naming conventions
  for colours, such as ``diffuse_red`` or ``r``/``g``/``b``, are not recognised: such
  files read as having no colours.
* From ``cwipc_metadata``: the cellsize, and the timestamp if the ``timestamp`` argument
  of :cpp:func:`cwipc_read` is 0. A non-zero ``timestamp`` argument always wins.

Binary little-endian files whose vertex element has exactly the ``struct cwipc_point``
layout (which includes all binary files cwipc writes, and those written by cwipc before
8.3.0) are read through a fast path, without parsing individual values: about ten times
faster than other binary PLY files.

cwipcdump files
---------------

A ``.cwipcdump`` file (:cpp:func:`cwipc_read_debugdump`, :cpp:func:`cwipc_write_debugdump`)
is a 32-byte header (``struct cwipc_cwipcdump_header``: magic, timestamp, cellsize, data
size) followed by the points in ``struct cwipc_point`` layout. It is fast but not readable
by other tools. Binary PLY files written by cwipc 8.3.0 and later carry the same
information; replacing cwipcdump by them is discussed in
`cwipc#338 <https://github.com/cwi-dis/cwipc/issues/338>`_.
