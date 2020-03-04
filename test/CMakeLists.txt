# Tests

macro(BUILD_TESTS name lib energymon_get_c)
  if (NOT ENERGYMON_BUILD_TESTS)
    return()
  endif()

  add_executable(${name}-interface-test ${CMAKE_SOURCE_DIR}/test/interface_test.c ${energymon_get_c})
  target_link_libraries(${name}-interface-test ${lib})

  add_executable(${name}-interval-test ${CMAKE_SOURCE_DIR}/test/interval_test.c ${energymon_get_c})
  target_link_libraries(${name}-interval-test ${lib})
endmacro(BUILD_TESTS)
