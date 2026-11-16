import sys
import os
import subprocess
from typing import List, Tuple
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