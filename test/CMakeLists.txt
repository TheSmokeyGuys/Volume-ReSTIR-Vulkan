if(WIN32)
  add_executable(openvdb_test WIN32 openvdb_test.cpp)
  target_link_libraries(openvdb_test ${WINLIBS})
  set_target_properties(openvdb_test 
    PROPERTIES 
    COMPILE_FLAGS "/EHsc /W4"
  )
else(WIN32)
  add_executable(openvdb_test openvdb_test.cpp)
  target_link_libraries(openvdb_test ${CMAKE_THREAD_LIBS_INIT})
  set_target_properties(openvdb_test
    PROPERTIES 
    COMPILE_FLAGS "-Wall"
  )
endif(WIN32)

target_link_libraries(openvdb_test
  OpenVDB::openvdb
)
