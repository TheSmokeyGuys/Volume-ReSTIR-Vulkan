file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VK_SDK_PATH}/Bin/ $ENV{VK_SDK_PATH}/Bin32/)

file(GLOB_RECURSE SHADER_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.geom
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.comp
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.tese
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.tesc
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.rchit
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.rahit
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.rint
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.rmiss
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.rgen
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.rcall
)

file(GLOB_RECURSE GLSL_HEADER_FILES 
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/headers/*.h 
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/headers/*.glsl
)

file(GLOB_RECURSE GLSL_STRUCTURE_FILES 
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/structs/*.h 
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/structs/*.glsl
)

source_group("Shaders" FILES ${SHADER_SOURCES})

message(STATUS "OpenVDB include dir: ${OpenVDB_INCLUDE_DIR}")

include_directories(
  SYSTEM PUBLIC
  ${GLM_INCLUDE_DIR}
  ${STB_INCLUDE_DIR}
  ${OpenVDB_INCLUDE_DIR}
)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(${PROJECT_NAME})

#--------------------------------------------------------------------------------------------------
# GLSL to SPIR-V custom build
#
message(STATUS "Compiling shader sources:")
set(SHADER_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
SET(VULKAN_TARGET_ENV vulkan1.2)
UNSET(SPV_OUTPUT)
foreach(GLSL ${SHADER_SOURCES})
  message(STATUS "\t${GLSL}")
  get_filename_component(FILE_NAME ${GLSL} NAME)
  _compile_GLSL(${GLSL} ${SHADER_DIR}/${FILE_NAME}.spv GLSL_SOURCES SPV_OUTPUT)
endforeach(GLSL)
# list(APPEND SHADER_SOURCES ${GLSL_HEADER_FILES})

message(STATUS "Shader output: ${SPV_OUTPUT}")

#--------------------------------------------------------------------------------------------------
# Sources
target_sources(${PROJECT_NAME} PUBLIC ${SOURCES})
target_sources(${PROJECT_NAME} PUBLIC ${PACKAGE_SOURCE_FILES})
target_sources(${PROJECT_NAME} PUBLIC ${GLSL_SOURCES})
target_sources(${PROJECT_NAME} PUBLIC ${GLSL_HEADER_FILES})
target_sources(${PROJECT_NAME} PUBLIC ${GLSL_STRUCTURE_FILES})

#--------------------------------------------------------------------------------------------------
# Sub-folders in Visual Studio
#
source_group("Sources"      FILES ${SOURCES})
source_group("Common"       FILES ${PACKAGE_SOURCE_FILES})
source_group("Shader_Files" FILES ${GLSL_SOURCES})
source_group("Shader_Headers" FILES ${GLSL_HEADER_FILES})
source_group("Shader_Structures" FILES ${GLSL_STRUCTURE_FILES})
#--------------------------------------------------------------------------------------------------

target_link_libraries(${PROJECT_NAME} 
  PUBLIC
  ${ASSIMP_LIBRARIES}
  ${PLATFORM_LIBRARIES} 
  shared_sources 
  spdlog::spdlog
  vk-bootstrap::vk-bootstrap
  OpenVDB::openvdb
)

foreach(DEBUGLIB ${LIBRARIES_DEBUG})
  target_link_libraries(${PROJECT_NAME} PUBLIC debug ${DEBUGLIB})
endforeach(DEBUGLIB)

foreach(RELEASELIB ${LIBRARIES_OPTIMIZED})
  target_link_libraries(${PROJECT_NAME} PUBLIC optimized ${RELEASELIB})
endforeach(RELEASELIB)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

IF(MSVC)
  target_compile_options(${PROJECT_NAME}
    PRIVATE
    /EHsc /W4
  )
ELSE(MSVC)
  target_compile_options(${PROJECT_NAME}
    PRIVATE
    -Wall -Werror -Wno-unused-variable -Wno-deprecated-declarations
  )
ENDIF(MSVC)

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/config/build_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config/build_config.h"
)
