cmake_minimum_required(VERSION 2.8)

project(energymon)
set(PROJECT_VERSION 0.1.0)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unknown-pragmas")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

set(PKG_CONFIG_EXEC_PREFIX "\${prefix}")
set(PKG_CONFIG_LIBDIR "\${prefix}/lib")
set(PKG_CONFIG_INCLUDEDIR "\${prefix}/include/energymon")
set(PKG_CONFIG_CFLAGS "-I\${includedir}")

include_directories(${PROJECT_SOURCE_DIR}/inc)
include_directories(${PROJECT_SOURCE_DIR}/src)

macro(SET_FPIC target)
  # Required to force fPIC on static libs
  # 'set (CMAKE_POSITION_INDEPENDENT_CODE TRUE)' not supported until cmake 2.8.9
  IF(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    SET_TARGET_PROPERTIES(${target} PROPERTIES COMPILE_FLAGS "-fPIC")
  ENDIF(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
endmacro(SET_FPIC)

macro(BUILD_DEFAULT source deplibs definitions)
  add_library(energymon-default SHARED ${source})
  target_link_libraries(energymon-default ${deplibs})
  SET_FPIC(energymon-default)

  add_library(energymon-default-static STATIC ${source})
  target_link_libraries(energymon-default-static ${deplibs})
  SET_FPIC(energymon-default-static)

  set_target_properties(energymon-default energymon-default-static PROPERTIES COMPILE_DEFINITIONS "ENERGYMON_DEFAULT;${definitions}")
endmacro(BUILD_DEFAULT)

macro(PKG_CONFIG name description dependencies)
  set(PKG_CONFIG_NAME "${name}")
  set(PKG_CONFIG_DESCRIPTION "${description}")
  set(PKG_CONFIG_LIBS "-L\${libdir} -l${name} ${dependencies}")
  configure_file(
    "${CMAKE_SOURCE_DIR}/pkgconfig.in"
    "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/${name}.pc"
  )
endmacro(PKG_CONFIG)

# Libraries

set(DEFAULT "dummy" CACHE STRING "Default EnergyMon implementation")

add_subdirectory(dummy)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  add_subdirectory(msr)
  add_subdirectory(odroid)
  add_subdirectory(osp)
  add_subdirectory(rapl)
endif()

# Binaries

add_executable(energymon src/app/energymon.c)
target_link_libraries(energymon energymon-default)

# Tests

add_executable(interface-test test/interface_test.c)
target_link_libraries(interface-test energymon-default)

add_executable(interval-test test/interval_test.c)
target_link_libraries(interval-test energymon-default)

# Install

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/inc/ DESTINATION include/${PROJECT_NAME})
install(TARGETS energymon DESTINATION bin)

# Uninstall

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
