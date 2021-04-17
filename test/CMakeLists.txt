# Tests

function(add_energymon_tests SHORT_NAME TARGET_LIB ENERGYMON_GET_C)
  if(NOT ENERGYMON_BUILD_TESTS)
    return()
  endif()

  if(${SHORT_NAME} STREQUAL "default")
    # special case - default implementation
    set(TEST_PREFIX "energymon")
  else()
    set(TEST_PREFIX "energymon-${SHORT_NAME}")
  endif()

  add_executable(${TEST_PREFIX}-interface-test ${PROJECT_SOURCE_DIR}/test/interface_test.c ${ENERGYMON_GET_C})
  target_include_directories(${TEST_PREFIX}-interface-test PRIVATE ${PROJECT_SOURCE_DIR}/common)
  target_link_libraries(${TEST_PREFIX}-interface-test PRIVATE ${TARGET_LIB})

  add_executable(${TEST_PREFIX}-interval-test ${PROJECT_SOURCE_DIR}/test/interval_test.c ${ENERGYMON_GET_C})
  target_include_directories(${TEST_PREFIX}-interval-test PRIVATE ${PROJECT_SOURCE_DIR}/common)
  target_link_libraries(${TEST_PREFIX}-interval-test PRIVATE ${TARGET_LIB})
endfunction(add_energymon_tests)
