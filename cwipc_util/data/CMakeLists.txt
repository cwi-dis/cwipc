cmake_minimum_required(VERSION 3.16.0)
set(all
	"readme.md"
	"target-a4-aruco-0.pdf"
	"target-a4-aruco-aux-1.pdf"
	"target-a4-aruco-aux-2.pdf"
	"target-a4-aruco-aux-3.pdf"
	"target-a4-aruco-aux-4.pdf"
	"target-a4-aruco-aux-5.pdf"
)
install(FILES ${all} DESTINATION ${CMAKE_REGISTRATIONDATA_INSTALL_DIRECTORY})