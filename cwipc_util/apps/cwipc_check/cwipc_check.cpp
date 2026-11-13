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

static int check() {
    bool ok = true;

    // Native modules.
    std::vector<fs::path> checks = install_check_programs();
    if (checks.empty()) {
        std::cerr << progName << ": no *_install_check programs found in " << (prefix / "libexec" / "cwipc").string() << " or " << exeDir.string() << std::endl;
        ok = false;
    }
    for (const fs::path& program : checks) {
        std::string module = program.stem().string();
        module = module.substr(0, module.size() - strlen("_install_check"));
        std::cerr << module << ": checking" << std::endl;
        int status = run(quote(program));
        if (status != 0) {
            std::cerr << module << ": not ok (exit status " << status << ")" << std::endl;
            ok = false;
        } else {
            std::cerr << module << ": ok" << std::endl;
        }
    }

    // Python modules, in the tree's venv.
    fs::path python = venv_bin() / (std::string("python") + EXE_SUFFIX);
    std::error_code ec;
    if (!fs::exists(python, ec)) {
        std::cerr << "python: no venv with the cwipc modules (" << python.string() << " not found): not ok" << std::endl;
        ok = false;
    } else {
        std::cerr << "python modules: checking" << std::endl;
        int status = run(quote(python) + " -c \"import cwipc; print('cwipc', cwipc.cwipc_get_version())\"");
        if (status != 0) {
            std::cerr << "python modules: not ok (exit status " << status << ")" << std::endl;
            ok = false;
        } else {
            std::cerr << "python modules: ok" << std::endl;
        }
        std::cerr << "command line utilities: checking" << std::endl;
        status = run(quote(venv_bin() / (std::string("cwipc_view") + EXE_SUFFIX)) + " --version");
        if (status != 0) {
            std::cerr << "command line utilities: not ok (exit status " << status << ")" << std::endl;
            ok = false;
        } else {
            std::cerr << "command line utilities: ok" << std::endl;
        }
    }
    return ok ? 0 : 1;
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
            std::cerr << std::endl << "Your cwipc installation appears to be fully functional." << std::endl;
        } else {
            std::cerr << std::endl << "There are problems with your cwipc installation; see the messages above." << std::endl;
#ifdef WIN32
            std::cerr << "Running \"cwipc_check install\", or \"Attempt to fix cwipc installation\" from the Start menu, may fix them." << std::endl;
#else
            std::cerr << "Running \"cwipc_check install\" may fix them." << std::endl;
#endif
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
