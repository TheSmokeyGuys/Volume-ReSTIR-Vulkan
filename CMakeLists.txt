cmake_minimum_required(VERSION 3.15)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
if(WIN32)
  # FIXME: how to find OpenVDB on Windows?
  # set(OpenVDB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}\\external\\OpenVDB\\include")
  # set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}\\external\\OpenVDB\\lib\\cmake\\OpenVDB")
else(WIN32)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "/usr/local/lib/cmake/OpenVDB")
endif(WIN32)

project(volume_restir)

OPTION(USE_D2D_WSI "Build the project using Direct to Display swapchain" OFF)

find_package(Vulkan REQUIRED)
# find_package(OpenVDB)

IF(WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
ELSE(WIN32)
  find_package(Threads REQUIRED)
IF(USE_D2D_WSI)
  MESSAGE(STATUS "Vulkan: Using direct to display extension -D_DIRECT2DISPLAY...")
  add_definitions(-D_DIRECT2DISPLAY)
ELSE(USE_D2D_WSI)
  message(STATUS "Vulkan: Using default WSI -DVK_USE_PLATFORM_XCB_KHR...")
  find_package(XCB REQUIRED)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_XCB_KHR")
ENDIF(USE_D2D_WSI)
    # Todo : android?
ENDIF(WIN32)

# Set preprocessor defines
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX -D_USE_MATH_DEFINES")

add_definitions(-D_CRT_SECURE_NO_WARNINGS)
set(CMAKE_CXX_STANDARD 17)

message(STATUS "Using C++ standard: ${CMAKE_CXX_STANDARD}")

# Enable the creation of folders for Visual Studio projects
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

function(ExternalTarget folder target)
  set_property(TARGET ${target} PROPERTY FOLDER ${folder})
endfunction(ExternalTarget)

function(InternalTarget folder target)
  ExternalTarget("${folder}" ${target})
  if (MSVC)
    get_target_property(targetSources ${target} SOURCES)
    foreach(sourceFile IN ITEMS ${targetSources})
      if (IS_ABSOLUTE "${sourceFile}")
        file(RELATIVE_PATH sourceFile "${CMAKE_CURRENT_SOURCE_DIR}" "${sourceFile}")
      endif()
      get_filename_component(sourceDir "${sourceFile}" PATH)
      string(REPLACE "/" "\\" sourceDir "${sourceDir}")
      source_group("${sourceDir}" FILES "${sourceFile}")
    endforeach()
  endif()
endfunction(InternalTarget)

IF(WIN32)
  # Nothing here (yet)
ELSE(WIN32)
  link_libraries(${XCB_LIBRARIES} ${VULKAN_LIB})
ENDIF(WIN32)

string(TOLOWER "${CMAKE_BUILD_TYPE}" project_build_type)
if(project_build_type MATCHES "debug")
  message(WARNING "Project is built in debug mode!")
  add_definitions(-DDEBUG)
else(project_build_type MATCHES "debug")
  message(STATUS "Building in release mode")
endif(project_build_type MATCHES "debug")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_subdirectory(external)
add_subdirectory(src)
# add_subdirectory(test)
