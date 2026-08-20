#include <iostream>
#include <fstream>
#include "string.h"
#include <stdlib.h>
#include <inttypes.h>
#ifdef WIN32
#include <windows.h>
#define SEP "\\"
#else
#define SEP "/"
#endif

std::string libExecDir(LIBEXECDIR);
std::string progName;

int check() {

    std::string cmd;
    int status;
    bool ok = true;
    
    std::cerr << "cwipc_util: checking installation: " << std::endl;
    cmd = "\"" + libExecDir + SEP "cwipc" SEP "cwipc_util_install_check" + "\"";
    status = ::system(cmd.c_str());
    if (status != 0) {
        ok = false;
        std::cerr << "cwipc_util: not ok." << std::endl;
        std::cerr << "cwipc_util: " << cmd << ": exit status: " << status << std::endl;
    }
    else 
    {
        std::cerr << "cwipc_util: ok." << std::endl;
    }
    
    std::cerr << "cwipc_codec: checking installation: " << std::endl;
    cmd = "\"" + libExecDir + SEP "cwipc" SEP "cwipc_codec_install_check" + "\"";
    status = ::system(cmd.c_str());
    if (status != 0) {
        ok = false;
        std::cerr << "cwipc_codec: not ok." << std::endl;
        std::cerr << "cwipc_codec: " << cmd << ": exit status: " << status << std::endl;
    }
    else
    {
        std::cerr << "cwipc_codec: ok." << std::endl;
    }

    std::cerr << "cwipc_realsense2: checking installation: " << std::endl;
    cmd = "\"" + libExecDir + SEP "cwipc" SEP "cwipc_realsense2_install_check" + "\"";
    status = ::system(cmd.c_str());
    if (status != 0) {
        ok = false;
        std::cerr << "cwipc_realsense2: not ok." << std::endl;
        std::cerr << "cwipc_realsense2: " << cmd << ": exit status: " << status << std::endl;
    }
    else
    {
        std::cerr << "cwipc_realsense2: ok." << std::endl;
    }

    std::cerr << "cwipc_kinect: checking installation: " << std::endl;
    cmd = "\"" + libExecDir + SEP "cwipc" SEP "cwipc_kinect_install_check" + "\"";
    status = ::system(cmd.c_str());
    if (status != 0) {
        ok = false;
        std::cerr << "cwipc_kinect: not ok." << std::endl;
        std::cerr << "cwipc_kinect: " << cmd << ": exit status: " << status << std::endl;
    }
    else
    {
        std::cerr << "cwipc_kinect: ok." << std::endl;
    }
#ifdef DISABLED
    // This needs to be done differently. We need to test using cwipc_python
    std::cerr << "python: determining version:" << std::endl;
    cmd = "python --version";
    status = ::system(cmd.c_str());
    if (status != 0) {
        ok = false;
        std::cerr << "python: not ok." << std::endl;
        std::cerr << "python: " << cmd << ": exit status: " << status << std::endl;
    }
    else
    {
        std::cerr << "python: ok." << std::endl;
    }
    std::cerr << "cwipc python modules: determining version:" << std::endl;
    cmd = "python -m cwipc.scripts.cwipc_view --version";
    status = ::system(cmd.c_str());
    if (status != 0) {
        ok = false;
        std::cerr << "cwipc python modules: not ok." << std::endl;
        std::cerr << "cwipc python modules: " << cmd << ": exit status: " << status << std::endl;
    }
    else
    {
        std::cerr << "cwipc python modules: ok." << std::endl;
    }
#endif
    // Test command line utilities
    std::cerr << "command line utilities: checking" << std::endl;
    cmd = "cwipc_view --version";
    status = ::system(cmd.c_str());
    if (status != 0) {
        ok = false;
        std::cerr << "command line utilities: not ok." << std::endl;
        std::cerr << "command line utilities: " << cmd << ": exit status: " << status << std::endl;
    }
    else
    {
        std::cerr << "command line utilities: ok." << std::endl;
    }
    if (!ok) return 1;
    return 0;
}

int install() {
#ifdef WIN32
    std::string script = "\"" + libExecDir + "\\cwipc\\scripts\\install-3rdparty-windows-asadmin.ps1" + "\"";

    std::string cmd = "powershell -ExecutionPolicy Bypass -File " + script;
    std::cerr << progName << ": execute command: " << cmd << std::endl;
    int status = ::system(cmd.c_str());
    if (status != 0) {
        return status;
    }
#if 0
    script = "\"" + libExecDir + "\\..\\bin\\cwipc_pymodules_install.ps1" + "\"";

    cmd = "powershell -ExecutionPolicy Bypass -File " + script;
    std::cerr << progName << ": execute command: " << cmd << std::endl;
    status = ::system(cmd.c_str());
    if (status != 0) {
        return status;
    }
#endif
    return 0;
#else
    std::cerr << progName << ": only implemented on Windows. On other platforms use your local package manager (brew, apt, etc)" << std::endl;
    return -1;
#endif
}

#ifdef WIN32
bool win_get_progname_and_libexec() {
    TCHAR moduleName[2048];
    if (!GetModuleFileName(NULL, moduleName, 2048))
        return false;
    progName = std::string(moduleName);
    // Search for last backslash. Note that the progName path is returned by a win32 API call
    // so we can safely assume it uses backslashes.
    size_t last_backslash_pos = progName.find_last_of('\\');
    if (last_backslash_pos == std::string::npos)
        return false;
    // We can now get the path to the bin directory.
    std::string progDir = progName.substr(0, last_backslash_pos);
    last_backslash_pos = progDir.find_last_of('\\');
    if (last_backslash_pos == std::string::npos)
        return false;
    // We can not get the topdir.
    std::string topDir;
    while (last_backslash_pos != std::string::npos) {
        topDir = progDir.substr(0, last_backslash_pos);
        // We can now construct the libexec dir.
        std::string candidateLibexecDir = topDir + "\\libexec";
        if (GetFileAttributes(candidateLibexecDir.c_str()) != INVALID_FILE_ATTRIBUTES) {
            libExecDir = candidateLibexecDir;
            return true;
        }
        // if it didn't exist we go up one directory.
        last_backslash_pos = topDir.find_last_of('\\');
    }
    return false;
}
#endif

int main(int argc, char** argv) {
    progName = argv[0];
#ifdef WIN32
    if (!win_get_progname_and_libexec()) {
        std::cerr << progName << ": cannot determine cwipc install directory. Attempting to continue." << std::endl;
    }
    else
#endif
    {
        std::cerr << progName << ": " << std::endl;
    }

    std::string command = "check";
    if (argc >= 2) {
        command = argv[1];
    }
    if (command == "check") {
        int sts = check();
        if (sts == 0) {
            std::cerr << std::endl << "Your cwipc installation appears to be fully functional." << std::endl;
        } else {
            std::cerr << std::endl << "There are problems with your cwipc installation." << std::endl;
            std::cerr << "There may be more information above, or you may have seen a dialog box with more information." << std::endl;
            std::cerr << "Running \"cwipc_check install\" from the command line or \"fix installation\" from the Start menu may fix things." << std::endl;
            std::cerr << "Otherwise see the readme file for more options." << std::endl;

            return sts;
        }
    }
    else if (command == "install") {
        int sts = install();
        std::cerr << progName << ": install: exit status " << sts << std::endl;
        return sts;
    }
    else {
        std::cerr << "Usage: " << progName << "[ check | install | help ]" << std::endl;
        std::cerr << "check   - Tries to determine whether all cwipc third party requirements have been installed correctly (default option)" << std::endl;
        std::cerr << "install - Tries to install all cwipc third party requirements." << std::endl;
        std::cerr << "          This should open a PowerShell window as Administrator. If anything goes wrong and you want to submit a bug report" << std::endl;
        std::cerr << "          please include the full content of this PowerShell window." << std::endl;
        std::cerr << "help    - this message" << std::endl << std::endl;
    }
}
