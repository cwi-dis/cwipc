
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
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "python3-pip, libc6 (>= 2.38), libegl1, libgcc-s1 (>= 3.0), libglfw3 (>= 3.0), libglu1-mesa | libglu1, libgomp1 (>= 4.9), libjpeg8 (>= 8c), libk4a1.3 (= 1.3.0), libk4abt1.0 (= 1.0.0), liblz4-1 (>= 0.0~r130), libopencv-core406t64 (>= 4.6.0+dfsg), libopencv-imgproc406t64 (>= 4.6.0+dfsg), libopengl0, libpcl-common1.14 (>= 1.14.0+dfsg), libpcl-io1.14 (>= 1.14.0+dfsg), libstdc++6 (>= 13.1), libturbojpeg (>= 1.2.90)")
    set(CPACK_DEBIAN_PACKAGE_RECOMMENDS " librealsense2 (>= 2.56.5-0~realsense.17055)")
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${_debdir}/postinst;${_debdir}/postrm")
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
    string(APPEND CPACK_NSIS_EXTRA_INSTALL_COMMANDS "ExecWait '\\\"$INSTDIR\\\\bin\\\\cwipc_check\\\" install'\\n")
    #string(APPEND CPACK_NSIS_EXTRA_INSTALL_COMMANDS "ExecWait 'powershell -ExecutionPolicy Bypass ${_hide} -File \\\"$INSTDIR\\\\bin\\\\cwipc_pymodules_install.ps1\\\"'\\n")
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "")
    string(APPEND CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "Delete $INSTDIR\\\\bin\\\\cwipc_*.exe\\n")
    string(APPEND CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "RMDir /r $INSTDIR\\\\libexec\\\\cwipc\\\\venv\\n")
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
