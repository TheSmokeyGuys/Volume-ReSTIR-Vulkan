file(GLOB_RECURSE VDB_SOURCES ${PROJECT_SOURCE_DIR}/src/vdb/*.cpp ${PROJECT_SOURCE_DIR}/src/vdb/*.h ${PROJECT_SOURCE_DIR}/src/vdb/*.hpp)

if(WIN32)
  add_executable(openvdb_test WIN32 openvdb_test.cpp ${VDB_SOURCES})
  target_link_libraries(openvdb_test ${WINLIBS})
  set_target_properties(openvdb_test 
    PROPERTIES 
    COMPILE_FLAGS "/EHsc /W4"
  )

  add_executable(openvdb_test_2 WIN32 openvdb_test_2.cpp ${VDB_SOURCES})
  target_link_libraries(openvdb_test_2 ${WINLIBS})
  set_target_properties(openvdb_test_2 
    PROPERTIES 
    COMPILE_FLAGS "/EHsc /W4"
  )
else(WIN32)
  add_executable(openvdb_test openvdb_test.cpp ${VDB_SOURCES})
  target_link_libraries(openvdb_test ${CMAKE_THREAD_LIBS_INIT})
  set_target_properties(openvdb_test
    PROPERTIES 
    COMPILE_FLAGS "-Wall"
  )
  
  add_executable(openvdb_test_2 openvdb_test_2.cpp ${VDB_SOURCES})
  target_link_libraries(openvdb_test_2 ${CMAKE_THREAD_LIBS_INIT})
  set_target_properties(openvdb_test_2
    PROPERTIES 
    COMPILE_FLAGS "-Wall"
  )
endif(WIN32)

include_directories(
  SYSTEM PUBLIC
  ${GLM_INCLUDE_DIR}
  ${STB_INCLUDE_DIR}
  ${OpenVDB_INCLUDE_DIR}
)

include_directories(
  ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(openvdb_test
  OpenVDB::openvdb
  spdlog::spdlog
)

target_link_libraries(openvdb_test_2
  OpenVDB::openvdb
  spdlog::spdlog
)
