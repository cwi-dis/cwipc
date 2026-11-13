//
// cwipc_check: check a cwipc installation, or fix it (cwi-dis/cwipc#355).
//
//   cwipc_check [check]   run the install checks of the native modules, and check the
//                         Python modules in the tree's venv
//   cwipc_check install   (re)run the tree's setup script, with dependencies
//
// The tree (prefix) is found from this program's own location: the first ancestor
// directory that has libexec/cwipc. That works for installed trees (installer, Debian
// package, unpacked archives, brew) and for a source tree (build/bin -> build).
//
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <cstring>

#ifdef WIN32
#include <windows.h>
#include <shlobj.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <unistd.h>
#include <sys/wait.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

namespace fs = std::filesystem;

static std::string progName = "cwipc_check";
static fs::path prefix;     // the tree
static fs::path exeDir;     // where this program is

#ifdef WIN32
static const char* EXE_SUFFIX = ".exe";
#else
static const char* EXE_SUFFIX = "";
#endif

// Path of this executable.
static fs::path exe_path() {
#ifdef WIN32
    wchar_t buf[4096];
    DWORD n = GetModuleFileNameW(NULL, buf, sizeof(buf) / sizeof(buf[0]));
    if (n == 0 || n >= sizeof(buf) / sizeof(buf[0])) return fs::path();
    return fs::path(buf);
#elif defined(__APPLE__)
    char buf[4096];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) != 0) return fs::path();
    std::error_code ec;
    fs::path p = fs::canonical(buf, ec);
    return ec ? fs::path(buf) : p;
#else
    std::error_code ec;
    fs::path p = fs::read_symlink("/proc/self/exe", ec);
    return ec ? fs::path() : p;
#endif
}

// Find the tree: the first ancestor of our directory with libexec/cwipc. Fall back on
// the libexec directory compiled in at configure time.
static bool find_prefix() {
    fs::path exe = exe_path();
    if (!exe.empty()) {
        exeDir = exe.parent_path();
        for (fs::path dir = exeDir; !dir.empty(); dir = dir.parent_path()) {
            std::error_code ec;
            if (fs::is_directory(dir / "libexec" / "cwipc", ec)) {
                prefix = dir;
                return true;
            }
            if (dir == dir.parent_path()) break;
        }
    }
#ifdef LIBEXECDIR
    fs::path compiled(LIBEXECDIR);
    std::error_code ec;
    if (fs::is_directory(compiled / "cwipc", ec)) {
        prefix = compiled.parent_path();
        return true;
    }
#endif
    return false;
}

// Quote a path for the shell (cmd.exe on Windows, sh elsewhere).
static std::string quote(const fs::path& p) {
#ifdef WIN32
    return "\"" + p.string() + "\"";
#else
    std::string s = p.string();
    std::string rv = "'";
    for (char c : s) {
        if (c == '\'') rv += "'\\''";
        else rv += c;
    }
    return rv + "'";
#endif
}

static int run(const std::string& cmd) {
    std::cerr << progName << ": running: " << cmd << std::endl;
#ifdef WIN32
    // cmd.exe strips the outer quotes of a command line that starts with a quote.
    int status = ::system(("\"" + cmd + "\"").c_str());
    return status;
#else
    int status = ::system(cmd.c_str());
    if (status == -1) return -1;
    return WIFEXITED(status) ? WEXITSTATUS(status) : 128;
#endif
}

// The *_install_check programs of the native modules: in libexec/cwipc in an installed
// tree, next to this program in a source tree.
static std::vector<fs::path> install_check_programs() {
    std::vector<fs::path> rv;
    std::vector<std::string> seen;
    for (const fs::path& dir : { prefix / "libexec" / "cwipc", exeDir }) {
        std::error_code ec;
        if (!fs::is_directory(dir, ec)) continue;
        for (const auto& entry : fs::directory_iterator(dir, ec)) {
            std::string name = entry.path().filename().string();
            std::string suffix = std::string("_install_check") + EXE_SUFFIX;
            if (name.rfind("cwipc_", 0) != 0) continue;
            if (name.size() < suffix.size() || name.compare(name.size() - suffix.size(), suffix.size(), suffix) != 0) continue;
            if (std::find(seen.begin(), seen.end(), name) != seen.end()) continue;
            seen.push_back(name);
            rv.push_back(entry.path());
        }
    }
    std::sort(rv.begin(), rv.end());
    return rv;
}

static fs::path venv_bin() {
#ifdef WIN32
    return prefix / "libexec" / "cwipc" / "venv" / "Scripts";
#else
    return prefix / "libexec" / "cwipc" / "venv" / "bin";
#endif
}

#ifdef WIN32
static const char PATH_SEP = ';';
#else
static const char PATH_SEP = ':';
#endif

static std::string get_env(const char* name) {
    const char* v = std::getenv(name);
    return v ? std::string(v) : std::string();
}

static void set_env(const std::string& name, const std::string& value) {
#ifdef WIN32
    _putenv_s(name.c_str(), value.c_str());
#else
    setenv(name.c_str(), value.c_str(), 1);
#endif
}

static void prepend_env(const std::string& name, const std::string& dir) {
    std::string old = get_env(name.c_str());
    set_env(name, old.empty() ? dir : dir + PATH_SEP + old);
}

// Give our child processes the tree's own environment, as its activate file would: from
// <prefix>/libexec/cwipc/environment, written by the setup script (format: see
// setup-cwipc.sh). Without that file: our own directory first on PATH.
static void set_tree_environment() {
    std::ifstream f(prefix / "libexec" / "cwipc" / "environment");
    if (!f) {
        prepend_env("PATH", exeDir.string());
        set_env("CWIPC_LIBRARY_DIR", exeDir.string());
        return;
    }
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.rfind("path ", 0) == 0) {
            prepend_env("PATH", line.substr(5));
        } else if (line.rfind("env ", 0) == 0) {
            size_t eq = line.find('=');
            if (eq != std::string::npos) set_env(line.substr(4, eq - 4), line.substr(eq + 1));
        } else if (line.rfind("libpath ", 0) == 0) {
            size_t sp = line.find(' ', 8);
            if (sp != std::string::npos) prepend_env(line.substr(8, sp - 8), line.substr(sp + 1));
        }
    }
}

// The first file called name in the directories on PATH, or empty.
static fs::path find_on_path(const std::string& name) {
    std::string path = get_env("PATH");
    size_t pos = 0;
    while (pos <= path.size()) {
        size_t next = path.find(PATH_SEP, pos);
        if (next == std::string::npos) next = path.size();
        std::string dir = path.substr(pos, next - pos);
        pos = next + 1;
        if (dir.empty()) continue;
        std::error_code ec;
        fs::path candidate = fs::path(dir) / name;
        if (fs::is_regular_file(candidate, ec)) return candidate;
    }
    return fs::path();
}

// Is found the same file as one of ours (after resolving symlinks, such as the Debian
// package's /usr/bin/cwipc -> /usr/libexec/cwipc/venv/bin/cwipc)?
static bool same_file_as_one_of(const fs::path& found, const std::vector<fs::path>& ours) {
    std::error_code ec;
    for (const fs::path& p : ours) {
        if (fs::exists(p, ec) && fs::equivalent(found, p, ec)) return true;
    }
    return false;
}

struct Check {
    std::string name;
    std::string cmd;
    // For a capturer: what it needs that is installed separately (or not at all). Empty
    // for the core modules and Python.
    std::string optional_needs;
};

// The capturers are optional: their vendor library may be missing (the Debian package
// only recommends librealsense2, which comes from the RealSense apt repository).
static std::string capturer_needs(const std::string& module) {
    if (module == "cwipc_realsense2") return "librealsense2, for RealSense cameras";
    if (module == "cwipc_kinect") return "the Azure Kinect SDK, for Kinect cameras";
    if (module == "cwipc_orbbec") return "the OrbbecSDK, for Orbbec cameras";
    return "";
}

// Did the program not even start because a shared library is missing? (The loader's exit
// status: 127 on Linux and macOS, STATUS_DLL_NOT_FOUND on Windows.)
static bool library_missing(int status) {
#ifdef WIN32
    return status == (int)0xC0000135;
#else
    return status == 127;
#endif
}

static std::vector<Check> all_checks() {
    std::vector<Check> rv;
    for (const fs::path& program : install_check_programs()) {
        std::string module = program.stem().string();
        module = module.substr(0, module.size() - strlen("_install_check"));
        rv.push_back({module, quote(program), capturer_needs(module)});
    }
    fs::path python = venv_bin() / (std::string("python") + EXE_SUFFIX);
    std::error_code ec;
    if (fs::exists(python, ec)) {
        rv.push_back({"python modules", quote(python) + " -c \"import cwipc; print('cwipc', cwipc.cwipc_get_version())\""});
        rv.push_back({"command line utilities", quote(venv_bin() / (std::string("cwipc_view") + EXE_SUFFIX)) + " --version"});
    }
    return rv;
}

struct CheckResults {
    std::vector<Check> failed;
    std::vector<Check> unavailable;   // optional, and a library it needs is missing
};

static CheckResults run_checks(const std::vector<Check>& checks) {
    CheckResults rv;
    for (const Check& c : checks) {
        std::cerr << c.name << ": checking" << std::endl;
        int status = run(c.cmd);
        if (status == 0) {
            std::cerr << c.name << ": ok" << std::endl;
        } else if (!c.optional_needs.empty() && library_missing(status)) {
            std::cerr << c.name << ": not available (a library it needs is missing; it needs " << c.optional_needs << ")" << std::endl;
            rv.unavailable.push_back(c);
        } else {
            std::cerr << c.name << ": not ok (exit status " << status << ")" << std::endl;
            rv.failed.push_back(c);
        }
    }
    return rv;
}

// cwipc_check checks whether cwipc works *from the environment it was started in* (a
// terminal, the Start menu): that decides the exit status. If something fails, it runs
// the failed checks again with the tree's own environment, to tell a broken installation
// from an environment that doesn't lead to it (cwi-dis/cwipc#355). A capturer whose
// vendor library is missing is reported as not available, which is not a failure.
static int check() {
    std::error_code ec;
    // broken: problems that no environment fixes.
    bool broken = false;

    std::vector<Check> checks = all_checks();
    if (install_check_programs().empty()) {
        std::cerr << progName << ": no *_install_check programs found in " << (prefix / "libexec" / "cwipc").string() << " or " << exeDir.string() << std::endl;
        broken = true;
    }
    fs::path python = venv_bin() / (std::string("python") + EXE_SUFFIX);
    if (!fs::exists(python, ec)) {
        std::cerr << "python: no venv with the cwipc modules (" << python.string() << " not found): not ok" << std::endl;
        broken = true;
    }
    CheckResults results = run_checks(checks);

    // What this environment leads to.
    std::cerr << std::endl << "environment: checking" << std::endl;
    bool system_install = fs::exists(prefix / "libexec" / "cwipc" / "launchers", ec);
    std::string cwipc_cmd = std::string("cwipc") + EXE_SUFFIX;
    fs::path found_cmd = find_on_path(cwipc_cmd);
    bool env_problem = false;
    if (found_cmd.empty()) {
        if (system_install) {
            std::cerr << "environment: the cwipc command is not on PATH: not ok" << std::endl;
            env_problem = true;
        } else {
            std::cerr << "environment: the cwipc command is not on PATH (normal for this kind of installation: use " << (prefix / "libexec" / "cwipc" / "activate").string() << ")" << std::endl;
        }
    } else if (same_file_as_one_of(found_cmd, { exeDir / cwipc_cmd, venv_bin() / cwipc_cmd })) {
        std::cerr << "environment: the cwipc command is " << found_cmd.string() << ", from this installation: ok" << std::endl;
    } else {
        std::cerr << "environment: the cwipc command is " << found_cmd.string() << ", from another installation: not ok" << std::endl;
        env_problem = true;
    }
#ifdef WIN32
    // Windows finds the DLLs through PATH, for the programs and for Python.
    fs::path found_dll = find_on_path("cwipc_util.dll");
    if (found_dll.empty()) {
        std::cerr << "environment: cwipc_util.dll is not on PATH: not ok" << std::endl;
        env_problem = true;
    } else if (same_file_as_one_of(found_dll, { exeDir / "cwipc_util.dll" })) {
        std::cerr << "environment: cwipc_util.dll is " << found_dll.string() << ", from this installation: ok" << std::endl;
    } else {
        std::cerr << "environment: cwipc_util.dll is " << found_dll.string() << ", from another installation: not ok" << std::endl;
        env_problem = true;
    }
#endif

    // Tell a broken installation from an environment that doesn't lead to it. Capturers
    // that weren't available go along: on Windows a missing PATH looks the same.
    std::vector<Check> again = results.failed;
    again.insert(again.end(), results.unavailable.begin(), results.unavailable.end());
    std::vector<Check> unavailable = results.unavailable;
    if (!again.empty()) {
        std::cerr << std::endl << "Checking again what failed, with the environment of this installation (" << prefix.string() << "):" << std::endl;
        set_tree_environment();
        CheckResults results2 = run_checks(again);
        if (!results2.failed.empty()) broken = true;
        // Something that works with the tree's environment but didn't in the caller's.
        if (results2.failed.size() + results2.unavailable.size() < again.size()) env_problem = true;
        unavailable = results2.unavailable;
    }
    std::cerr << std::endl;
    for (const Check& c : unavailable) {
        std::cerr << c.name << " is not available; it needs " << c.optional_needs << " (optional)." << std::endl;
    }
    if (broken) {
        std::cerr << "The installation itself is broken; see the messages above." << std::endl;
#ifdef WIN32
        std::cerr << "Running \"cwipc_check install\", or \"Attempt to fix cwipc installation\" from the Start menu, may fix it." << std::endl;
#else
        std::cerr << "Running \"cwipc_check install\" may fix it." << std::endl;
#endif
        return 1;
    }
    if (env_problem) {
        std::cerr << "The installation itself works, but not from this environment (see \"environment:\" above)." << std::endl;
#ifdef WIN32
        std::cerr << "If you have just installed cwipc: open a new terminal window. Otherwise, check PATH," << std::endl;
        std::cerr << "or use: & \"" << (prefix / "libexec" / "cwipc" / "activate.ps1").string() << "\"" << std::endl;
#else
        std::cerr << "Check PATH, or use: source " << quote(prefix / "libexec" / "cwipc" / "activate") << std::endl;
#endif
        return 1;
    }
    return 0;
}

static int install() {
    std::error_code ec;
    fs::path libexec = prefix / "libexec" / "cwipc";
    bool launchers = fs::exists(libexec / "launchers", ec);
#ifdef WIN32
    // The Windows installer and "fix installation" in the Start menu: copy the launchers
    // into bin, so the Start menu shortcuts work.
    launchers = true;
    fs::path script = libexec / "scripts" / "setup-cwipc.ps1";
    if (!fs::exists(script, ec)) {
        std::cerr << progName << ": " << script.string() << " not found" << std::endl;
        return 1;
    }
    std::string args = std::string("-Deps") + (launchers ? " -Launchers" : "");
    if (IsUserAnAdmin()) {
        // From the installer (which runs as administrator): run it here, no extra window.
        return run("powershell -NoProfile -ExecutionPolicy Bypass -File " + quote(script) + " " + args);
    }
    // Not elevated: Windows can only elevate a new process, which gets its own window.
    // -PauseOnError keeps that window open only if something fails.
    std::string inner = "-NoProfile -ExecutionPolicy Bypass -File \\\"" + script.string() + "\\\" " + args + " -PauseOnError";
    return run("powershell -NoProfile -Command \"$p = Start-Process powershell -Verb RunAs -Wait -PassThru -ArgumentList '" + inner + "'; exit $p.ExitCode\"");
#else
    std::string prefixString = prefix.string();
    if (prefixString.find("/Cellar/") != std::string::npos) {
        std::cerr << progName << ": this cwipc was installed with Homebrew, which manages its venv." << std::endl;
        std::cerr << progName << ": to fix it: brew reinstall cwipc" << std::endl;
        return 1;
    }
    // Installed tree: the copy of the setup script in the tree. Source tree (prefix is
    // <repo>/build): <repo>/scripts.
    fs::path script = libexec / "scripts" / "setup-cwipc.sh";
    if (!fs::exists(script, ec)) script = prefix.parent_path() / "scripts" / "setup-cwipc.sh";
    if (!fs::exists(script, ec)) {
        std::cerr << progName << ": setup-cwipc.sh not found" << std::endl;
        return 1;
    }
    if (prefix == fs::path("/usr")) launchers = true;   // the Debian package
    std::string cmd = "bash " + quote(script) + " --prefix " + quote(prefix) + " --deps" + (launchers ? " --launchers" : "");
    bool writable = access((fs::exists(libexec, ec) ? libexec : prefix).c_str(), W_OK) == 0;
    if (!writable) {
        std::cerr << progName << ": " << prefixString << " is not writable, using sudo" << std::endl;
        cmd = "sudo " + cmd;
    }
    return run(cmd);
#endif
}

static void usage() {
    std::cerr << "Usage: " << progName << " [ check | install | help ]" << std::endl;
    std::cerr << "check   - Check whether cwipc and its dependencies are installed correctly (default)." << std::endl;
    std::cerr << "install - Fix the installation: install the dependencies and set up the Python modules," << std::endl;
    std::cerr << "          by running the setup script of this cwipc tree." << std::endl;
#ifdef WIN32
    std::cerr << "          This asks for administrator rights if needed, and then opens a window." << std::endl;
#else
    std::cerr << "          Uses sudo if the tree isn't writable (for example the Debian package in /usr)." << std::endl;
#endif
}

int main(int argc, char** argv) {
    if (!find_prefix()) {
        std::cerr << progName << ": cannot determine where cwipc is installed (no libexec/cwipc above " << exeDir.string() << ")" << std::endl;
        return 2;
    }
    std::cerr << progName << ": cwipc in " << prefix.string() << std::endl;

    std::string command = argc >= 2 ? argv[1] : "check";
    if (command == "check") {
        int sts = check();
        if (sts == 0) {
            std::cerr << "Your cwipc installation appears to be fully functional." << std::endl;
        }
        return sts;
    }
    if (command == "install") {
        int sts = install();
        std::cerr << progName << ": install: exit status " << sts << std::endl;
        return sts;
    }
    usage();
    return command == "help" ? 0 : 2;
}
