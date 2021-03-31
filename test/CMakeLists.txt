# Tests

function(BUILD_TESTS name lib energymon_get_c)
  if (NOT ENERGYMON_BUILD_TESTS)
    return()
  endif()

  if(${name} STREQUAL "default")
    # special case - default implementation
    set(TEST_PREFIX "energymon")
  else()
    set(TEST_PREFIX "energymon-${name}")
  endif()

  add_executable(${TEST_PREFIX}-interface-test ${CMAKE_SOURCE_DIR}/test/interface_test.c ${energymon_get_c})
  target_link_libraries(${TEST_PREFIX}-interface-test PRIVATE ${lib})

  add_executable(${TEST_PREFIX}-interval-test ${CMAKE_SOURCE_DIR}/test/interval_test.c ${energymon_get_c})
  target_link_libraries(${TEST_PREFIX}-interval-test PRIVATE ${lib})
endfunction(BUILD_TESTS)
