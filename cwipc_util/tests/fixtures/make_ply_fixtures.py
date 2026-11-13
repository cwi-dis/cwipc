"""Generate the PLY fixtures in input/ply/ for the PLY reader/writer tests (#66).

Usage:
    python make_ply_fixtures.py            # hand-made layout variants (pure Python)
    python make_ply_fixtures.py --legacy   # also the legacy_* files (needs cwipc)

The hand-made variants all contain the same two points (VARIANT_POINTS), in
different PLY layouts, so the tests can check each of them against known values.

The legacy_* files are point clouds as written by cwipc before 8.3.0, which used
PCL's PLYWriter: the fourth byte of cwipc_point is stored as `alpha`, and there is
an `element camera` after the vertices. They are generated with the PCL-based
cwipc_write/cwipc_write_ext and so can only be regenerated with a cwipc before
8.3.0. legacy.cwipcdump holds the same points, as reference.
"""
import sys
import os
import struct

OUTDIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "input", "ply")

# x, y, z, r, g, b, tile
VARIANT_POINTS = [
    (1.0, 2.0, 3.0, 255, 128, 0, 5),
    (-1.5, 0.25, 9.0, 10, 20, 30, 2),
]

# Metadata for the cwipc_metadata variants: timestamp in seconds, cellsize
VARIANT_TIMESTAMP_SECONDS = 1731490000.123
VARIANT_CELLSIZE = 0.0025

XYZ = ["float x", "float y", "float z"]
RGB = ["uchar red", "uchar green", "uchar blue"]
VERTEX_ALPHA = XYZ + RGB + ["uchar alpha"]
VERTEX_TILE = XYZ + RGB + ["uchar tile"]
METADATA = "element cwipc_metadata 1\nproperty double timestamp\nproperty float cellsize\n"

def _vertex_header(props):
    return "element vertex %d\n" % len(VARIANT_POINTS) + "".join("property %s\n" % p for p in props)

def write_ascii(name, props, rowfn, before="", after="", trailer=""):
    """ASCII file. before/after are extra header elements before/after the vertex element,
    trailer is the data for the extra elements (written after the vertex data)."""
    with open(os.path.join(OUTDIR, name), "w", newline="\n") as f:
        f.write("ply\nformat ascii 1.0\n" + before + _vertex_header(props) + after + "end_header\n")
        for p in VARIANT_POINTS:
            f.write(" ".join(repr(v) for v in rowfn(p)) + "\n")
        f.write(trailer)

def write_binary(name, endian, vertex_props, fmt, before=b"", before_header="", after=b"", after_header=""):
    formatname = {"<": "binary_little_endian", ">": "binary_big_endian"}[endian]
    with open(os.path.join(OUTDIR, name), "wb") as f:
        header = "ply\nformat %s 1.0\n" % formatname + before_header + _vertex_header(vertex_props) + after_header + "end_header\n"
        f.write(header.encode("ascii"))
        f.write(before)
        for p in VARIANT_POINTS:
            f.write(struct.pack(endian + fmt, *p))
        f.write(after)

# Gaussian splat layout, as written by the reference implementation
# (graphdeco-inria/gaussian-splatting, scene/gaussian_model.py): all float.
SPLAT_PROPS = ["x", "y", "z", "nx", "ny", "nz"] + ["f_dc_%d" % i for i in range(3)] + \
    ["f_rest_%d" % i for i in range(45)] + ["opacity"] + ["scale_%d" % i for i in range(3)] + ["rot_%d" % i for i in range(4)]
SPLAT_C0 = 0.28209479177387814

def make_splat(name):
    with open(os.path.join(OUTDIR, name), "wb") as f:
        header = "ply\nformat binary_little_endian 1.0\nelement vertex %d\n" % len(VARIANT_POINTS)
        header += "".join("property float %s\n" % p for p in SPLAT_PROPS) + "end_header\n"
        f.write(header.encode("ascii"))
        for p in VARIANT_POINTS:
            f_dc = [(c / 255 - 0.5) / SPLAT_C0 for c in p[3:6]]
            values = list(p[:3]) + [0.0] * 3 + f_dc + [0.0] * 45 + [5.0] + [-5.0] * 3 + [1.0, 0.0, 0.0, 0.0]
            assert len(values) == len(SPLAT_PROPS)
            f.write(struct.pack("<%df" % len(values), *values))

def make_variants():
    os.makedirs(OUTDIR, exist_ok=True)
    full = lambda p: p
    notile = lambda p: p[:6]
    meta_ascii = "%r %r\n" % (VARIANT_TIMESTAMP_SECONDS, VARIANT_CELLSIZE)
    meta_bin_le = struct.pack("<df", VARIANT_TIMESTAMP_SECONDS, VARIANT_CELLSIZE)

    # Layouts the reader accepts, with the same result as the PCL-based reader
    write_ascii("ascii_alpha.ply", VERTEX_ALPHA, full)
    write_ascii("ascii_noalpha.ply", XYZ + RGB, notile)
    write_ascii("ascii_nocolor.ply", XYZ, lambda p: p[:3])
    write_ascii("ascii_double_xyz.ply", ["double x", "double y", "double z"] + RGB + ["uchar alpha"], full)
    write_ascii("ascii_reordered.ply", RGB + ["uchar alpha"] + XYZ, lambda p: p[3:7] + p[:3])
    write_ascii("ascii_extra_props.ply", XYZ + ["float nx", "float ny", "float nz"] + RGB + ["uchar alpha"],
        lambda p: p[:3] + (0.0, 0.0, 1.0) + p[3:7])
    write_ascii("ascii_typenames.ply", ["float32 x", "float32 y", "float32 z", "uint8 red", "uint8 green", "uint8 blue", "uint8 alpha"], full)
    write_ascii("ascii_face_element.ply", VERTEX_ALPHA, full, after="element face 0\nproperty list uchar int vertex_indices\n")
    # Windows line endings (CRLF), in header and data
    write_ascii("ascii_crlf.ply", VERTEX_ALPHA, full)
    crlf = os.path.join(OUTDIR, "ascii_crlf.ply")
    with open(crlf, "rb") as f:
        data = f.read()
    with open(crlf, "wb") as f:
        f.write(data.replace(b"\n", b"\r\n"))
    write_binary("binary_le_alpha.ply", "<", VERTEX_ALPHA, "fffBBBB")
    write_binary("binary_be_alpha.ply", ">", VERTEX_ALPHA, "fffBBBB")
    write_binary("binary_le_double_xyz.ply", "<", ["double x", "double y", "double z"] + RGB + ["uchar alpha"], "dddBBBB")

    # Layouts whose result changes with the new reader (#66)
    write_ascii("ascii_tile.ply", VERTEX_TILE, full)
    write_binary("binary_le_tile.ply", "<", VERTEX_TILE, "fffBBBB")
    write_ascii("ascii_metadata_after.ply", VERTEX_TILE, full, after=METADATA, trailer=meta_ascii)
    write_binary("binary_le_metadata_after.ply", "<", VERTEX_TILE, "fffBBBB", after=meta_bin_le, after_header=METADATA)
    write_binary("binary_le_metadata_before.ply", "<", VERTEX_TILE, "fffBBBB", before=meta_bin_le, before_header=METADATA)

    # Colours the new reader can't use: points read with black colours and a log warning.
    # (The PCL-based reader gives black without a warning.) Other tools' colour names such as
    # diffuse_red or r/g/b are not here: cwipc_read treats them as unknown properties (no colour).
    write_ascii("ascii_float_colors.ply", XYZ + ["float red", "float green", "float blue"],
        lambda p: p[:3] + tuple(c / 255 for c in p[3:6]))
    write_ascii("ascii_ushort_colors.ply", XYZ + ["ushort red", "ushort green", "ushort blue"],
        lambda p: p[:3] + tuple(c * 257 for c in p[3:6]))
    make_splat("binary_le_splat.ply")

    # Not PLY files at all
    with open(os.path.join(OUTDIR, "bad_magic.ply"), "w") as f:
        f.write("plx\nformat ascii 1.0\nelement vertex 0\nend_header\n")
    with open(os.path.join(OUTDIR, "bad_truncated.ply"), "w") as f:
        f.write("ply\nformat ascii 1.0\n" + _vertex_header(VERTEX_ALPHA) + "end_header\n1 2 3 4 5 6 7\n")
    with open(os.path.join(OUTDIR, "bad_noxyz.ply"), "w") as f:
        f.write("ply\nformat ascii 1.0\nelement vertex 1\nproperty uchar red\nproperty uchar green\nproperty uchar blue\nend_header\n1 2 3\n")

def make_legacy():
    import cwipc
    loot = os.path.join(os.path.dirname(__file__), "..", "..", "..", "cwipc_codec", "tests", "fixtures", "input", "loot_tiled.ply")
    pc = cwipc.cwipc_read(loot, 0)
    points = pc.get_points()
    step = len(points) // 1000
    sample = [(p.x, p.y, p.z, p.r, p.g, p.b, p.tile) for p in list(points)[::step]]
    small = cwipc.cwipc_from_points(cwipc.cwipc_point_array(values=sample), 0)
    cwipc.cwipc_write(os.path.join(OUTDIR, "legacy_pcl_ascii.ply"), small)
    cwipc.cwipc_write(os.path.join(OUTDIR, "legacy_pcl_binary.ply"), small, cwipc.CWIPC_FLAGS_BINARY)
    cwipc.cwipc_write_debugdump(os.path.join(OUTDIR, "legacy.cwipcdump"), small)
    print("legacy: %d points, tiles %s" % (len(sample), sorted(set(p[6] for p in sample))))

if __name__ == "__main__":
    make_variants()
    if "--legacy" in sys.argv:
        make_legacy()
