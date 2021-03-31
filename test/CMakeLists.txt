# Tests

function(BUILD_TESTS name incdirs lib energymon_get_c)
  if (NOT ENERGYMON_BUILD_TESTS)
    return()
  endif()

  if(${name} STREQUAL "default")
    # special case - default implementation
    set(TEST_PREFIX "energymon")
  else()
    set(TEST_PREFIX "energymon-${name}")
  endif()

  add_executable(${TEST_PREFIX}-interface-test ${PROJECT_SOURCE_DIR}/test/interface_test.c ${energymon_get_c})
  target_include_directories(${TEST_PREFIX}-interface-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                                                                   ${PROJECT_SOURCE_DIR}/inc
                                                                   ${PROJECT_SOURCE_DIR}/common)
  target_include_directories(${TEST_PREFIX}-interface-test PRIVATE ${incdirs})
  target_link_libraries(${TEST_PREFIX}-interface-test PRIVATE ${lib})

  add_executable(${TEST_PREFIX}-interval-test ${PROJECT_SOURCE_DIR}/test/interval_test.c ${energymon_get_c})
  target_include_directories(${TEST_PREFIX}-interval-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                                                                  ${PROJECT_SOURCE_DIR}/inc
                                                                  ${PROJECT_SOURCE_DIR}/common)
  target_include_directories(${TEST_PREFIX}-interval-test PRIVATE ${incdirs})
  target_link_libraries(${TEST_PREFIX}-interval-test PRIVATE ${lib})
endfunction(BUILD_TESTS)
