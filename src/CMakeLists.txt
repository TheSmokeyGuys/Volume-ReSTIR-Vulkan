file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VK_SDK_PATH}/Bin/ $ENV{VK_SDK_PATH}/Bin32/)

file(GLOB_RECURSE SHADER_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.vert
  ${CMAKE_CURRENT_SOURCE_DIR}/*.frag
  ${CMAKE_CURRENT_SOURCE_DIR}/*.geom
  ${CMAKE_CURRENT_SOURCE_DIR}/*.comp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.tese
  ${CMAKE_CURRENT_SOURCE_DIR}/*.tesc
)

source_group("Shaders" FILES ${SHADER_SOURCES})

if(WIN32)
  add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${SHADER_SOURCES})
  target_link_libraries(${PROJECT_NAME} ${WINLIBS})
else(WIN32)
  add_executable(${PROJECT_NAME} ${SOURCES})
  target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
endif(WIN32)

message(STATUS "Compiling shader sources:")

foreach(SHADER_SOURCE ${SHADER_SOURCES})
  message(STATUS "\t${SHADER_SOURCE}")

  set(SHADER_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)

  get_filename_component(fname ${SHADER_SOURCE} NAME)
  add_custom_target(${fname}.spv
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_DIR} && 
    ${GLSL_VALIDATOR} -V ${SHADER_SOURCE} -o ${SHADER_DIR}/${fname}.spv
    SOURCES ${SHADER_SOURCE}
  )

  ExternalTarget("Shaders" ${fname}.spv)
  add_dependencies(${PROJECT_NAME} ${fname}.spv)
endforeach()

target_link_libraries(${PROJECT_NAME} 
  ${ASSIMP_LIBRARIES}
  Vulkan::Vulkan 
  glfw
  spdlog::spdlog
  vk-bootstrap::vk-bootstrap
  OpenVDB::openvdb
)
target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${GLM_INCLUDE_DIR}
  ${STB_INCLUDE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

IF(MSVC)
  target_compile_options(${PROJECT_NAME}
    PRIVATE
    /EHsc /W4 /WX
  )
ELSE(MSVC)
  target_compile_options(${PROJECT_NAME}
    PRIVATE
    -Wall -Werror -Wno-unused-variable
  )
ENDIF(MSVC)

InternalTarget("" ${PROJECT_NAME})

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/config/build_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config/build_config.h"
)
