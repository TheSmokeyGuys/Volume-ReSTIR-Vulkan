file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VK_SDK_PATH}/Bin/ $ENV{VK_SDK_PATH}/Bin32/)

file(GLOB_RECURSE SHADER_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.vert
  ${CMAKE_CURRENT_SOURCE_DIR}/*.frag
  ${CMAKE_CURRENT_SOURCE_DIR}/*.geom
  ${CMAKE_CURRENT_SOURCE_DIR}/*.comp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.tese
  ${CMAKE_CURRENT_SOURCE_DIR}/*.tesc
  ${CMAKE_CURRENT_SOURCE_DIR}/*.rchit
  ${CMAKE_CURRENT_SOURCE_DIR}/*.rahit
  ${CMAKE_CURRENT_SOURCE_DIR}/*.rint
  ${CMAKE_CURRENT_SOURCE_DIR}/*.rmiss
  ${CMAKE_CURRENT_SOURCE_DIR}/*.rgen
  ${CMAKE_CURRENT_SOURCE_DIR}/*.rcall
)

file(GLOB_RECURSE GLSL_HEADER_FILES 
  ${CMAKE_CURRENT_SOURCE_DIR}/headers/*.h 
  ${CMAKE_CURRENT_SOURCE_DIR}/headers/*.glsl
)

file(GLOB_RECURSE GLSL_STRUCTURE_FILES 
  ${CMAKE_CURRENT_SOURCE_DIR}/structs/*.h 
  ${CMAKE_CURRENT_SOURCE_DIR}/structs/*.glsl
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
_add_project_definitions(${PROJECT_NAME})


#--------------------------------------------------------------------------------------------------
# GLSL to SPIR-V custom build
#
set(SHADER_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
compile_glsl_directory(
	SRC "${SHADER_SOURCES}" 
	DST "${SHADER_DIR}" 
	VULKAN_TARGET "vulkan1.2"
	DEPENDENCY ${VULKAN_BUILD_DEPENDENCIES}
)

#--------------------------------------------------------------------------------------------------
# Sources
target_sources(${PROJECT_NAME} PUBLIC ${SOURCES})
target_sources(${PROJECT_NAME} PUBLIC ${COMMON_SOURCE_FILES})
target_sources(${PROJECT_NAME} PUBLIC ${PACKAGE_SOURCE_FILES})
target_sources(${PROJECT_NAME} PUBLIC ${SHADER_SOURCES} ${GLSL_HEADER_FILES} ${GLSL_STRUCTURE_FILES})


#--------------------------------------------------------------------------------------------------
# Sub-folders in Visual Studio
#
source_group("Common"       FILES ${COMMON_SOURCE_FILES} ${PACKAGE_SOURCE_FILES})
source_group("Sources"      FILES ${SOURCES})
source_group("Shader Sources"  FILES ${SHADER_SOURCES})
source_group("Shader Headers"  FILES ${GLSL_HEADER_FILES})
source_group("Shader Structs"  FILES ${GLSL_STRUCTURE_FILES})

#--------------------------------------------------------------------------------------------------
# Linkage
#
target_link_libraries(${PROJECT_NAME} 
  PUBLIC
  ${ASSIMP_LIBRARIES}
  ${PLATFORM_LIBRARIES} 
  nvpro_core 
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

InternalTarget("" ${PROJECT_NAME})

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/config/build_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config/build_config.h"
)
