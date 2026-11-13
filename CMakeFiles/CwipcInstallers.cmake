
# Creating installers
if(ANDROID)
    set(CPACK_GENERATOR TGZ)
    set(CPACK_SOURCE_GENERATOR TGZ)
elseif(APPLE)
    set(CPACK_GENERATOR TGZ)
    set(CPACK_SOURCE_GENERATOR TGZ)
elseif(UNIX)
    set(CPACK_GENERATOR DEB)
    set(CPACK_SOURCE_GENERATOR TGZ)
    set(_debdir "${CMAKE_CURRENT_LIST_DIR}/CwipcInstallers-debian")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Jack Jansen")
    # Enable this line to auto-detect dependencies (see toplevel readme):
    #set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS YES)
    # Enable these two lines after you have found the dependencies:
    # From dpkg-shlibdeps on a 24.04 build of 4cd6db16 (2026-11-13, cwi-dis/cwipc#355, #357), plus
    # python3-venv for the postinst. Ubuntu links with --as-needed, so of PCL only
    # libpcl-common is needed at runtime. librealsense2 comes from the RealSense apt
    # repository, so it's a recommendation; without it only cwipc_realsense2 doesn't load.
    # The second line: what the pip wheels the postinst installs into the venv (open3d,
    # PySide6 with Qt's X11 and Wayland plugins, opencv-python, ...) need from the system,
    # which dpkg-shlibdeps can't see (procedure: docs/distribution.rst, cwi-dis/cwipc#355).
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "python3-venv, libc6 (>= 2.38), libgcc-s1 (>= 3.0), libglfw3 (>= 3.0), libglu1-mesa | libglu1, libgomp1 (>= 4.9), libjpeg8 (>= 8c), liblz4-1 (>= 0.0~r130), libopengl0, libpcl-common1.14 (>= 1.14.0+dfsg), libstdc++6 (>= 13.1)")
    string(APPEND CPACK_DEBIAN_PACKAGE_DEPENDS ", libdbus-1-3, libegl1, libfontconfig1, libglib2.0-0t64, libidn2-0, libusb-1.0-0, libwayland-cursor0, libxcb-cursor0, libxcb-icccm4, libxcb-image0, libxcb-keysyms1, libxcb-render-util0, libxcb-shape0, libxkbcommon-x11-0")
    set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "librealsense2 (>= 2.59.1)")
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${_debdir}/postinst;${_debdir}/prerm")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
elseif(WIN32)
    set(CPACK_GENERATOR NSIS)
    set(CPACK_SOURCE_GENERATOR ZIP)
    set(_nsisdir "${CMAKE_CURRENT_LIST_DIR}/CwipcInstallers-nsis")
    set(CPACK_NSIS_PACKAGE_NAME "cwipc")
	set(CPACK_NSIS_INSTALL_DIRECTORY "cwipc")
    set(CPACK_NSIS_INSTALL_REGISTRY_KEY "cwipc")
    set(CPACK_NSIS_MODIFY_PATH YES)
	set(CPACK_NSIS_DEFINES "RequestExecutionLevel admin")
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "")
	#string(APPEND CPACK_NSIS_EXTRA_INSTALL_COMMANDS "LogSet On\\n")
	set(_hide "")
	#set(_hide "-WindowStyle Hidden")
    string(APPEND CPACK_NSIS_EXTRA_INSTALL_COMMANDS "nsExec::ExecToLog '\\\"$INSTDIR\\\\bin\\\\cwipc_check\\\" install'\\nPop $0\\nDetailPrint \\\"cwipc_check returned $0\\\"")
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "")
    # Remove what setup-cwipc.ps1 created (cwi-dis/cwipc#355): the launchers it copied into bin
    # (cwipc*.exe, so including cwipc.exe, which cwipc_*.exe missed and left the install
    # directory behind), the venv and the other per-tree files.
    string(APPEND CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "Delete $INSTDIR\\\\bin\\\\cwipc*.exe\\n")
    string(APPEND CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "RMDir /r $INSTDIR\\\\libexec\\\\cwipc\\\\venv\\n")
    foreach(_f activate activate.ps1 environment launchers)
        string(APPEND CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "Delete $INSTDIR\\\\libexec\\\\cwipc\\\\${_f}\\n")
    endforeach()
	set(CPACK_NSIS_MENU_LINKS
        "libexec/cwipc/scripts/run-cwipc-view-synthetic.bat" "View a sample dynamic pointcloud"
        "libexec/cwipc/scripts/run-cwipc-check-fix.bat" "Attempt to fix cwipc installation"
        "libexec/cwipc/scripts/run-cwipc-check.bat" "Check cwipc installation"
        "share/docs/cwipc/readme.md" "Readme file"
		)
else()
    message(WARNING Cannot create packages for this system)
endif()
set(CPACK_PACKAGE_VENDOR "CWI DIS Group")
set(CPACK_PACKAGE_CONTACT "Jack.Jansen@cwi.nl")
set(CPACK_PACKAGE_VERSION ${CWIPC_VERSION})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/readme.md")
set(CPACK_RESOURCE_FILE_WELCOME "${CMAKE_CURRENT_SOURCE_DIR}/readme.md")
set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/package")
set(CPACK_PACKAGE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} _arch)
string(TOLOWER ${CMAKE_SYSTEM_NAME} _sys)
string(TOLOWER ${PROJECT_NAME} _project_lower)
set(CPACK_PACKAGE_FILE_NAME "${_project_lower}-${CWIPC_VERSION}-${_sys}-${_arch}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${_project_lower}-${CWIPC_VERSION}")

# not .gitignore as its regex syntax is distinct
file(READ ${CMAKE_CURRENT_LIST_DIR}/.cpack_ignore _cpack_ignore)
string(REGEX REPLACE "\n" ";" _cpack_ignore ${_cpack_ignore})
set(CPACK_SOURCE_IGNORE_FILES "${_cpack_ignore};vcpkg/buildtrees;vcpkg/packages;vcpkg/downloads")

install(FILES ${CPACK_RESOURCE_FILE_README} ${CPACK_RESOURCE_FILE_LICENSE}
  DESTINATION share/docs/${PROJECT_NAME})

include(CPack)
