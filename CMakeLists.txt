cmake_minimum_required(VERSION 3.9.6 FATAL_ERROR)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
if(WIN32)
  message(STATUS "Using system: Windows")
  SET(Vcpkg_ROOT "C:\\vcpkg" CACHE STRING "Root dir for Vcpkg")
  message(STATUS "Using Vcpkg_ROOT: ${Vcpkg_ROOT}")
  set(CMAKE_TOOLCHAIN_FILE "${Vcpkg_ROOT}\\scripts\\buildsystems\\vcpkg.cmake")
endif(WIN32)
if(UNIX AND NOT APPLE)
  message(STATUS "Using system: Linux")
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "/usr/local/lib/cmake/OpenVDB")
endif(UNIX AND NOT APPLE)

project(vk_raytracing_vdb)

#--------------------------------------------------------------------------------------------------
# look for nvpro_core 1) as a sub-folder 2) at some other locations
# this cannot be put anywhere else since we still didn't find setup.cmake yet
if(NOT BASE_DIRECTORY)

  find_path(BASE_DIRECTORY
    NAMES nvpro_core/cmake/setup.cmake
    PATHS ${PROJECT_SOURCE_DIR}/external
    REQUIRED
    DOC "Directory containing nvpro_core"
    )
endif()

## Various functions and macros REQUIRED
if(EXISTS ${BASE_DIRECTORY}/nvpro_core/cmake/setup.cmake)
  include(${BASE_DIRECTORY}/nvpro_core/cmake/setup.cmake)
  include(${BASE_DIRECTORY}/nvpro_core/cmake/utilities.cmake)
else()
  message(FATAL_ERROR "could not find base directory, please set BASE_DIRECTORY to folder containing nvpro_core")
endif()

set(TUTO_KHR_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    add_definitions(/wd26812)  # 'enum class' over 'enum'
    add_definitions(/wd26451)  # Arithmetic overflow, casting 4 byte value to 8 byte value
endif()

#--------------------------------------------------------------------------------------------------
# Package shared by all projects
_add_package_VulkanSDK()
_add_package_ImGUI()
_add_nvpro_core_lib()

if(APPLE)
  message(STATUS "Using system: MacOS")
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "/opt/homebrew/lib/cmake/OpenVDB/")
endif(APPLE)

message(STATUS "CMAKE_MODULE_PATH:")
foreach(MODULE_PATH ${CMAKE_MODULE_PATH})
  message("\t${MODULE_PATH}")
endforeach(MODULE_PATH)

find_package(OpenVDB REQUIRED)

message(STATUS "COPY ${CMAKE_CURRENT_SOURCE_DIR}/media  to  ${OUTPUT_PATH}")
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/media DESTINATION ${OUTPUT_PATH})



#--------------------------------------------------------------------------------------------------
# Install - copying the media directory
install(DIRECTORY "media" 
        CONFIGURATIONS Release 
        DESTINATION "bin_${ARCH}")
install(DIRECTORY "media" 
        CONFIGURATIONS Debug 
        DESTINATION "bin_${ARCH}_debug")

#--------------------------------------------------------------------------------------------------
# Sub examples
add_subdirectory(src)
add_subdirectory(external)