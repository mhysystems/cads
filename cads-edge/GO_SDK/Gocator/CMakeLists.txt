cmake_minimum_required(VERSION 3.10)
include(ExternalProject)

ExternalProject_Add(GoSdk_build
	CONFIGURE_COMMAND ""
	SOURCE_DIR ${CMAKE_SOURCE_DIR}
	BUILD_COMMAND make -C ${CMAKE_CURRENT_SOURCE_DIR} -f GoSdk-Linux_X64.mk config=Release
	INSTALL_COMMAND ""
)
