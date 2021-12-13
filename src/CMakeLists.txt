#*****************************************************************************
# Copyright 2020 NVIDIA Corporation. All rights reserved.
#*****************************************************************************

cmake_minimum_required(VERSION 3.9.6 FATAL_ERROR)

#--------------------------------------------------------------------------------------------------
# Project setting
message(STATUS "-------------------------------")
message(STATUS "Processing Project ${PROJECT_NAME}:")


#--------------------------------------------------------------------------------------------------
# C++ target and defines
set(CMAKE_CXX_STANDARD 17)
add_executable(${PROJECT_NAME})
_add_project_definitions(${PROJECT_NAME})


#--------------------------------------------------------------------------------------------------
# Source files for this project
#
file(GLOB_RECURSE SOURCE_FILES *.cpp *.hpp *.inl *.h *.c)
file(GLOB EXTRA_COMMON ${TUTO_KHR_DIR}/src/common/*.*)
list(APPEND COMMON_SOURCE_FILES ${EXTRA_COMMON})

message(STATUS "Choosing TUTO_KHR_DIR: ${TUTO_KHR_DIR}")

include_directories(
  SYSTEM PUBLIC
  ${OpenVDB_INCLUDE_DIR}
)

include_directories(
  ${TUTO_KHR_DIR}/src/common
  ${CMAKE_CURRENT_SOURCE_DIR}
)


#--------------------------------------------------------------------------------------------------
# GLSL to SPIR-V custom build
compile_glsl_directory(
	SRC "${CMAKE_CURRENT_SOURCE_DIR}/shaders" 
	DST "${CMAKE_CURRENT_SOURCE_DIR}/spv" 
	VULKAN_TARGET "vulkan1.2"
	DEPENDENCY ${VULKAN_BUILD_DEPENDENCIES}
)


#--------------------------------------------------------------------------------------------------
# Sources
target_sources(${PROJECT_NAME} PUBLIC ${SOURCE_FILES} ${HEADER_FILES})
target_sources(${PROJECT_NAME} PUBLIC ${COMMON_SOURCE_FILES})
target_sources(${PROJECT_NAME} PUBLIC ${PACKAGE_SOURCE_FILES})
target_sources(${PROJECT_NAME} PUBLIC ${GLSL_SOURCES} ${GLSL_HEADERS})


#--------------------------------------------------------------------------------------------------
# Sub-folders in Visual Studio
#
source_group("Common"       FILES ${COMMON_SOURCE_FILES} ${PACKAGE_SOURCE_FILES})
source_group("Sources"      FILES ${SOURCE_FILES})
source_group("Headers"      FILES ${HEADER_FILES})
source_group("Shader Sources"  FILES ${GLSL_SOURCES})
source_group("Shader Headers"  FILES ${GLSL_HEADERS})


#--------------------------------------------------------------------------------------------------
# Linkage
#
target_link_libraries(${PROJECT_NAME} 
  PUBLIC
  ${PLATFORM_LIBRARIES} 
  nvpro_core
)
target_link_libraries(${PROJECT_NAME} 
  PRIVATE
  spdlog::spdlog
  OpenVDB::openvdb
)

foreach(DEBUGLIB ${LIBRARIES_DEBUG})
  target_link_libraries(${PROJECT_NAME} PUBLIC debug ${DEBUGLIB})
endforeach(DEBUGLIB)

foreach(RELEASELIB ${LIBRARIES_OPTIMIZED})
  target_link_libraries(${PROJECT_NAME} PUBLIC optimized ${RELEASELIB})
endforeach(RELEASELIB)

#--------------------------------------------------------------------------------------------------
# Configure CMake Env Variables
#
configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/config/build_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config/build_config.h"
)

#--------------------------------------------------------------------------------------------------
# copies binaries that need to be put next to the exe files (ZLib, etc.)
#
_finalize_target( ${PROJECT_NAME} )


install(FILES ${SPV_OUTPUT} CONFIGURATIONS Release DESTINATION "bin_${ARCH}/${PROJECT_NAME}/spv")
install(FILES ${SPV_OUTPUT} CONFIGURATIONS Debug DESTINATION "bin_${ARCH}_debug/${PROJECT_NAME}/spv")
