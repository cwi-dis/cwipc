"""
Bind a Python to a cwipc tree: write, show or remove cwipc.pth in its site-packages (cwi-dis/cwipc#377).

The .pth file sets CWIPC_LIBRARY_DIR, unless already set, at every start of that Python, so
the cwipc Python modules load the native libraries of that tree. Its one line only acts if
the directory still exists.

Run as a script with the Python to bind (it imports nothing from cwipc, so it also works
before the cwipc modules are installed, or when they would load the wrong libraries):

    python _binding.py write LIBRARY_DIR
    python _binding.py show
    python _binding.py remove

Used by `cwipc pipinstall` (another Python) and setup-cwipc (a source tree's own venv).
"""
import os
import sys
import sysconfig
from typing import Optional

PTH_NAME = "cwipc.pth"


def pth_path() -> str:
    return os.path.join(sysconfig.get_paths()["purelib"], PTH_NAME)


def pth_line(library_dir: str) -> str:
    d = repr(library_dir)
    return f'import os; os.path.isdir({d}) and os.environ.setdefault("CWIPC_LIBRARY_DIR", {d})\n'


def bound_to() -> Optional[str]:
    """The library directory the existing cwipc.pth binds to, or None."""
    try:
        with open(pth_path()) as fp:
            line = fp.readline()
    except OSError:
        return None
    marker = 'os.environ.setdefault("CWIPC_LIBRARY_DIR", '
    i = line.find(marker)
    if i < 0:
        return None
    rest = line[i + len(marker):].rstrip().rstrip(")")
    try:
        import ast
        return ast.literal_eval(rest)
    except (ValueError, SyntaxError):
        return None


def write(library_dir: str) -> None:
    library_dir = os.path.abspath(library_dir)
    old = bound_to()
    with open(pth_path(), "w") as fp:
        fp.write(pth_line(library_dir))
    if old and os.path.normcase(old) != os.path.normcase(library_dir):
        print(f"{PTH_NAME}: was bound to {old}, now to {library_dir}")


def remove() -> None:
    try:
        os.remove(pth_path())
    except FileNotFoundError:
        pass


def main() -> int:
    if len(sys.argv) == 3 and sys.argv[1] == "write":
        write(sys.argv[2])
    elif len(sys.argv) == 2 and sys.argv[1] == "show":
        print(bound_to() or "")
    elif len(sys.argv) == 2 and sys.argv[1] == "remove":
        remove()
    else:
        print(f"Usage: {sys.argv[0]} write LIBRARY_DIR | show | remove", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
