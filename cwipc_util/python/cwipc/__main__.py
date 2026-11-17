import sys
import os
import subprocess
from typing import Dict, List, Optional, Tuple
import pkgutil
from . import scripts, cwipc_get_version

MAIN_COMMANDS = ['view', 'grab', 'play', 'copy', 'register']

if sys.platform == 'win32':
    def execv(cmd : str, args : List[str]):
        sts = subprocess.run(args, executable=cmd)
        sys.exit(sts.returncode)
else:
    from os import execv

def find_scripts() -> Tuple[List[str], List[str]]:
    scripts_list = []
    for _, name, _ in pkgutil.iter_modules(scripts.__path__):
        if not name.startswith('cwipc_'):
            continue
        command = name[6:]
        if command in MAIN_COMMANDS:
            continue
        scripts_list.append(command)
    return MAIN_COMMANDS, scripts_list

def get_docstring(name : str) -> str:
    mod = __import__(f'cwipc.scripts.cwipc_{name}', fromlist=[''])
    if mod.__doc__ is None:
        return ""
    return mod.__doc__.strip().split('\n')[0]
    
def help():
    main_scripts, auxiliary_scripts = find_scripts()
    print(f"{sys.argv[0]} - CWI Point Cloud command line utility", file=sys.stderr)
    print("\nMain commands:", file=sys.stderr)
    for s in main_scripts:
        doc = get_docstring(s)
        if doc:
            print(f"  {s:20} - {doc}", file=sys.stderr)
        else:
            print(f"  {s:20}", file=sys.stderr)
    print("\nAdditional commands:", file=sys.stderr)
    for s in auxiliary_scripts:
        doc = get_docstring(s)
        if doc:
            print(f"  {s:20} - {doc}", file=sys.stderr)
        else:
            print(f"  {s:20}", file=sys.stderr)
    print("\nSpecial commands:", file=sys.stderr)
    print("  help                 - show this help message", file=sys.stderr)
    print("  version              - show cwipc version", file=sys.stderr)
    print("  check                - check if cwipc is correctly installed, or fix installation", file=sys.stderr)
    print("  python               - run python that has cwipc package installed", file=sys.stderr)
    print("  parallel             - run multiple commands in parallel (for testing)", file=sys.stderr)
    print("  pipinstall           - install the cwipc Python modules into another Python (such as your own venv)", file=sys.stderr)
    print("\nUse 'cwipc <command> -h' for help on a specific command.", file=sys.stderr)
    print("See http://github.com/cwi-dis/cwipc for more information.", file=sys.stderr)

def run_version() -> int:
    print(cwipc_get_version())
    return 0

def run_check() -> int:
    mydir = os.path.dirname(__file__)
    while True:
        totry = [
            os.path.join(mydir, 'bin', 'cwipc_check.exe'),
            os.path.join(mydir, 'bin', 'cwipc_check'),
            os.path.join(mydir, 'build', 'bin', 'cwipc_check'),
            os.path.join(mydir, 'build', 'bin', 'RelWithDebInfo', 'cwipc_check.exe'),
            os.path.join(mydir, 'build', 'bin', 'Release', 'cwipc_check.exe'),
        ]
        for path in totry:
            if os.path.exists(path):
                check_script = path
                execv(check_script, [check_script] + sys.argv[2:])
                assert 0, "execv failed"
        parent = os.path.dirname(mydir)
        if parent == mydir:
            print("Could not find cwipc_check executable", file=sys.stderr)
            return 1
        mydir = parent
        return 0
    
def run_python() -> int:
    execv(sys.executable, [sys.executable] + sys.argv[2:])
    assert 0, "execv failed"

PARALLEL_GRACE = 30.0     # seconds the others get after the first child ends
PARALLEL_TIMEOUT = 300.0  # seconds before the first child must have ended

def run_parallel() -> int:
    """Run commands in parallel, for tests: cwipc parallel [options] cmd [arg ...] -- cmd [arg ...] [-- ...]

    Each command is a cwipc subcommand (view, forward, ...), run as `cwipc <subcommand> ...`,
    or `! command ...` for any other command, found on PATH (a server from another package,
    for example). The `!` is a separate word, so interactive shells don't take it for history
    expansion. A command that cannot be started fails with status 127.
    All children are started at once (clients retry connecting until their server is up).
    The run ends when the first child ends: the others get a grace period to end by
    themselves, and are killed after it. The result is the status of the first child that
    failed, or 1 if a child had to be killed (cwi-dis/cwipc#333).
    """
    from ._runloop import Runloop
    usage = "Usage: cwipc parallel [--grace SECONDS] [--timeout SECONDS] cmd [arg ...] -- cmd [arg ...] [-- cmd [arg ...] ...]\n  cmd is a cwipc subcommand (view, forward, ...), or ! followed by any other command"
    cmd = sys.argv[0]
    args = sys.argv[2:]
    grace = PARALLEL_GRACE
    timeout = PARALLEL_TIMEOUT
    while args and args[0] in ('--grace', '--timeout'):
        if len(args) < 2:
            print(usage, file=sys.stderr)
            return 1
        try:
            value = float(args[1])
        except ValueError:
            print(usage, file=sys.stderr)
            return 1
        if args[0] == '--grace':
            grace = value
        else:
            timeout = value
        args = args[2:]
    commands : List[List[str]] = [[]]
    for a in args:
        if a == '--':
            commands.append([])
        else:
            commands[-1].append(a)
    if len(commands) < 2 or any(not c for c in commands) or any(c == ['!'] for c in commands):
        print(usage, file=sys.stderr)
        return 1
    main_scripts, auxiliary_scripts = find_scripts()
    subcommands = set(main_scripts) | set(auxiliary_scripts) | {'version', 'check', 'python', 'parallel'}
    for c in commands:
        if c[0] != '!' and c[0] not in subcommands:
            print(f"parallel: unknown cwipc subcommand '{c[0]}' (use ! {c[0]} ... for another command)", file=sys.stderr)
            return 1

    runloop = Runloop("parallel")
    killed : List[str] = []
    try:
        for i, c in enumerate(commands, start=1):
            full_cmd = c[1:] if c[0] == '!' else [cmd] + c
            try:
                proc = subprocess.Popen(full_cmd)
            except OSError as e:
                runloop.add_participant(str(i))
                runloop.post(str(i), 127, f"cannot start {full_cmd[0]}: {e}")
                continue
            runloop.add_process(str(i), proc)
            print(f"parallel: started {i}: {' '.join(full_cmd)}", file=sys.stderr, flush=True)
        first = runloop.next_ending(timeout=timeout)
        if first is None:
            print(f"parallel: no command ended within {timeout:g} s, killing all", file=sys.stderr, flush=True)
            killed = runloop.running()
        else:
            print(f"parallel: {first.who}: {first.reason}", file=sys.stderr, flush=True)
            if runloop.running():
                if first.status == 0:
                    print(f"parallel: waiting up to {grace:g} s for the others to end", file=sys.stderr, flush=True)
                else:
                    print(f"parallel: giving the others {grace:g} s to end", file=sys.stderr, flush=True)
                killed = runloop.wait_for_rest(grace)
            for e in runloop.endings[1:]:
                print(f"parallel: {e.who}: {e.reason}", file=sys.stderr, flush=True)
        for who in killed:
            print(f"parallel: {who}: still running, killed", file=sys.stderr, flush=True)
            runloop.stop(who)
        # Collect the exits of the killed ones, so no zombies are left.
        runloop.wait_for_rest(10)
    except KeyboardInterrupt:
        print("parallel: interrupted, killing all", file=sys.stderr, flush=True)
        runloop.stop_all()
        return 130
    for e in runloop.endings:
        if e.status != 0 and e.who not in killed:
            return e.status
    return 1 if killed else 0
    
PIPINSTALL_PACKAGES = ['cwipc_util', 'cwipc_codec', 'cwipc_realsense2', 'cwipc_kinect', 'cwipc_orbbec']

def run_pipinstall() -> int:
    """Install this cwipc's Python modules into another Python and bind it to this cwipc
    (cwi-dis/cwipc#377): cwipc pipinstall [--python PYTHON] [--editable] [--force] [--uninstall]"""
    import argparse
    import shutil
    parser = argparse.ArgumentParser(prog="cwipc pipinstall", description="Install the Python modules of this cwipc into another Python (by default the python on PATH, such as your own activated venv), and bind that Python to this cwipc's native libraries (with a cwipc.pth file in its site-packages).")
    parser.add_argument("--python", metavar="PYTHON", help="The Python to install into (default: python, or python3, on PATH)")
    parser.add_argument("--editable", action="store_true", help="Source tree only: install the modules editable from the source tree, instead of the wheels")
    parser.add_argument("--force", action="store_true", help="Also install into a Python that is not a virtual environment (venv or conda), or that is older than this cwipc needs")
    parser.add_argument("--uninstall", action="store_true", help="Remove the cwipc modules and the binding from that Python")
    args = parser.parse_args(sys.argv[2:])
    me = "cwipc pipinstall"

    target = args.python or shutil.which("python") or shutil.which("python3")
    if not target:
        print(f"{me}: no python or python3 on PATH; use --python", file=sys.stderr)
        return 1
    def target_run(*cmd : str, check : bool = True, env : Optional[Dict[str, str]] = None) -> str:
        r = subprocess.run([target, *cmd], capture_output=True, text=True, env=env)
        if check and r.returncode != 0:
            raise RuntimeError(f"{target} {' '.join(cmd)}: {(r.stderr or r.stdout).strip()}")
        return r.stdout.strip()
    try:
        target_prefix = target_run("-c", "import sys; print(sys.prefix)")
        target_version = target_run("-c", "import sys; print('%d.%d' % sys.version_info[:2])")
        target_is_env = target_run("-c", "import os, sys; print(sys.prefix != sys.base_prefix or os.path.isdir(os.path.join(sys.prefix, 'conda-meta')))") == "True"
    except (OSError, RuntimeError) as e:
        print(f"{me}: cannot run {target}: {e}", file=sys.stderr)
        return 1
    if os.path.realpath(target_prefix) == os.path.realpath(sys.prefix):
        print(f"{me}: {target} is the Python of this cwipc itself, nothing to do. Activate your own Python first, or use --python.", file=sys.stderr)
        return 1
    binding = os.path.join(os.path.dirname(os.path.abspath(__file__)), "_binding.py")
    if not target_is_env and not args.uninstall:
        if not args.force:
            print(f"{me}: {target} is not a virtual environment (venv or conda). Create one (python -m venv ...) and activate it, or use --force to install into this Python as a whole.", file=sys.stderr)
            return 1
        print(f"{me}: warning: {target} is not a virtual environment; installing into it as a whole (--force)", file=sys.stderr)

    if args.uninstall:
        target_run("-m", "pip", "uninstall", "--yes", *PIPINSTALL_PACKAGES, check=False)
        target_run(binding, "remove")
        print(f"{me}: removed the cwipc modules and the binding from {target}")
        return 0

    # This cwipc: the directory of the native library this command uses, and the wheels
    # in share/cwipc/python of the tree it is in (a few levels up on Windows: bin/<config>).
    from .util import cwipc_util_dll_load, cwipc_get_version
    libdir = os.path.dirname(os.path.abspath(cwipc_util_dll_load()._name))
    tree = libdir
    for _ in range(3):
        tree = os.path.dirname(tree)
        if os.path.isdir(os.path.join(tree, "share", "cwipc", "python")):
            break
    else:
        print(f"{me}: no share/cwipc/python found above {libdir}: is this a cwipc installation or build?", file=sys.stderr)
        return 1
    wheeldir = os.path.join(tree, "share", "cwipc", "python")
    srcdir = os.path.dirname(tree)
    is_source_tree = os.path.isfile(os.path.join(srcdir, "CMakeLists.txt")) and os.path.isdir(os.path.join(srcdir, "cwipc_util", "python"))

    # The Python versions (versions.env of the tree, or of the source tree): PYTHON_VERSION_MIN
    # is the oldest cwipc works with; PYTHON_VERSION the one its own venv is built and tested
    # with. Newer Pythons are fine if all dependencies (open3d, ...) have wheels for them,
    # which pip decides: open3d supports newer Pythons on most platforms, but on Intel Macs
    # only up to 3.12 (cwi-dis/cwipc#378).
    for candidate in (os.path.join(tree, "libexec", "cwipc", "scripts", "versions.env"), os.path.join(srcdir, "scripts", "versions.env")):
        if os.path.isfile(candidate):
            versions = {}
            with open(candidate) as fp:
                for line in fp:
                    if "=" in line and not line.lstrip().startswith("#"):
                        k, v = line.strip().split("=", 1)
                        versions[k] = v
            vmin, vtested = versions.get("PYTHON_VERSION_MIN"), versions.get("PYTHON_VERSION")
            as_tuple = lambda v: tuple(int(x) for x in v.split("."))
            if vmin and as_tuple(target_version) < as_tuple(vmin):
                if not args.force:
                    print(f"{me}: {target} is Python {target_version}, but this cwipc needs Python {vmin} or newer (--force to try anyway)", file=sys.stderr)
                    return 1
                print(f"{me}: warning: {target} is Python {target_version}, this cwipc needs {vmin} or newer; trying anyway (--force)", file=sys.stderr)
            elif vtested and as_tuple(target_version) > as_tuple(vtested):
                print(f"{me}: note: this cwipc is built and tested with Python {vtested}; Python {target_version} should work if all its dependencies support it (pip will tell)")
            break

    if args.editable:
        if not is_source_tree:
            print(f"{me}: --editable needs a source tree; this cwipc is installed in {tree}", file=sys.stderr)
            return 1
        packages = []
        for p in PIPINSTALL_PACKAGES:
            d = os.path.join(srcdir, p, "python")
            if os.path.isdir(d) and any(f.startswith(p + "-") for f in os.listdir(wheeldir)):
                packages += ["--editable", d]
        pip_args = ["-m", "pip", "install", *packages]
    else:
        wheels = sorted(os.path.join(wheeldir, f) for f in os.listdir(wheeldir) if f.startswith("cwipc_") and f.endswith(".whl"))
        if not wheels:
            print(f"{me}: no cwipc wheels in {wheeldir}", file=sys.stderr)
            return 1
        pip_args = ["-m", "pip", "install", "--upgrade", f"--find-links={wheeldir}", *wheels]
    print(f"{me}: installing the cwipc Python modules of {tree} into {target} (Python {target_version})", flush=True)
    r = subprocess.run([target, *pip_args])
    if r.returncode != 0:
        print(f"{me}: pip failed, see its messages above", file=sys.stderr)
        return 1
    try:
        target_run(binding, "write", libdir)
    except RuntimeError as e:
        print(f"{me}: installed the modules, but could not bind {target} to this cwipc: {e}", file=sys.stderr)
        return 1

    # Check, without CWIPC_LIBRARY_DIR from this environment, that the binding works.
    env = {k: v for k, v in os.environ.items() if k not in ("CWIPC_LIBRARY_DIR", "DYLD_LIBRARY_PATH", "LD_LIBRARY_PATH")}
    check = subprocess.run([target, "-W", "error::UserWarning", "-c", "import cwipc; print(cwipc.cwipc_get_version())"], capture_output=True, text=True, env=env)
    if check.returncode != 0:
        print(f"{me}: installed, but importing cwipc in {target} fails:\n{check.stderr.strip()}", file=sys.stderr)
        return 1
    print(f"{me}: done: {target} now uses cwipc {check.stdout.strip()} from {libdir}")
    return 0

def main():
    if len(sys.argv) < 2 or sys.argv[1] in ('-h', '--help', 'help'):
        help()
        sys.exit(1)
    command = sys.argv[1]
    if command in ('-v', '--version', 'version'):
        sys.exit(run_version())
    if command == "check":
        sys.exit(run_check())
    if command == "python":
        sys.exit(run_python())
    if command == "parallel":
        sys.exit(run_parallel())
    if command == "pipinstall":
        sys.exit(run_pipinstall())
    try:
        mod = __import__(f'cwipc.scripts.cwipc_{command}', fromlist=[''])
    except ModuleNotFoundError:
        print(f"Unknown command '{command}'. Use -h for help.")
        sys.exit(1)
    sys.argv[0] = sys.argv[0] + ' ' + command
    del sys.argv[1]
    sys.exit(mod.main())
    
if __name__ == '__main__':
    main()