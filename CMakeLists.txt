cmake_minimum_required(VERSION 2.8)

project(energymon)
set(PROJECT_VERSION 0.2.0)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=gnu99")

set(ENERGYMON_UTIL ${CMAKE_SOURCE_DIR}/common/energymon-util.c)
set(ENERGYMON_TIME_UTIL ${CMAKE_SOURCE_DIR}/common/energymon-time-util.c;${CMAKE_SOURCE_DIR}/common/ptime/ptime.c)

set(ENERGYMON_BUILD_LIB "ALL" CACHE STRING "Specify a single library to build, ALL, or NONE")
set(ENERGYMON_BUILD_DEFAULT "dummy" CACHE STRING "Default EnergyMon implementation, or NONE to disable building energymon-default")
if (DEFAULT AND NOT ENERGYMON_BUILD_DEFAULT)
  # for backward compatibility with old variable "DEFAULT"
  set(ENERGYMON_BUILD_DEFAULT DEFAULT)
endif()
set(ENERGYMON_BUILD_SHMEM_PROVIDERS TRUE CACHE BOOL "Enable/disable building shared memory providers")
set(ENERGYMON_BUILD_UTILITIES TRUE CACHE BOOL "Enable/disable building utility applications")
set(ENERGYMON_BUILD_TESTS TRUE CACHE BOOL "Enable/disable building tests")
set(ENERGYMON_BUILD_EXAMPLES TRUE CACHE BOOL "Enable/disable building example code")


if(UNIX AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  # Determine if we should link with librt for libraries that use "clock_gettime"
  include(CheckFunctionExists)
  CHECK_FUNCTION_EXISTS(clock_gettime HAVE_CLOCK_GETTIME)
  if(NOT HAVE_CLOCK_GETTIME)
    find_library(LIBRT NAMES rt)
  endif()
endif()

if(UNIX)
  set(LIBM m)
endif()

include_directories(${PROJECT_SOURCE_DIR}/inc)
include_directories(${PROJECT_SOURCE_DIR}/common)


# Macros

macro(BUILD_DEFAULT source deplibs definitions)
  add_library(energymon-default ${source})
  target_link_libraries(energymon-default ${deplibs})
  set_target_properties(energymon-default PROPERTIES COMPILE_DEFINITIONS "ENERGYMON_DEFAULT;${definitions}")
  install(TARGETS energymon-default DESTINATION lib)
  install(FILES ${CMAKE_SOURCE_DIR}/inc/energymon-default.h DESTINATION include/${PROJECT_NAME})
endmacro(BUILD_DEFAULT)

macro(PKG_CONFIG name description requires_private libs_private)
  set(PKG_CONFIG_PREFIX "${CMAKE_INSTALL_PREFIX}")
  set(PKG_CONFIG_EXEC_PREFIX "\${prefix}")
  set(PKG_CONFIG_LIBDIR "\${prefix}/lib")
  set(PKG_CONFIG_INCLUDEDIR "\${prefix}/include/${PROJECT_NAME}")
  set(PKG_CONFIG_NAME "${name}")
  set(PKG_CONFIG_DESCRIPTION "${description}")
  set(PKG_CONFIG_REQUIRES_PRIVATE "${requires_private}")
  set(PKG_CONFIG_CFLAGS "-I\${includedir}")
  set(PKG_CONFIG_LIBS "-L\${libdir} -l${name}")
  set(PKG_CONFIG_LIBS_PRIVATE "${libs_private}")
  configure_file(
    ${CMAKE_SOURCE_DIR}/pkgconfig.in
    ${CMAKE_CURRENT_BINARY_DIR}/${name}.pc
    @ONLY
  )
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${name}.pc DESTINATION lib/pkgconfig)
endmacro(PKG_CONFIG)


# Libraries

add_subdirectory(dummy)

if(UNIX)
  add_subdirectory(shmem)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux|Android")
  add_subdirectory(cray-pm)
  add_subdirectory(msr)
  add_subdirectory(odroid)
  add_subdirectory(rapl)
endif()

if(UNIX OR WIN32)
  add_subdirectory(osp)
  add_subdirectory(wattsup)
endif()


# Binaries and tests

if(TARGET energymon-default)
  if (ENERGYMON_BUILD_UTILITIES)
    add_subdirectory(utils)
  endif()
  if (ENERGYMON_BUILD_TESTS)
    add_subdirectory(test)
  endif()
endif()


# Install

install(FILES inc/energymon.h DESTINATION include/${PROJECT_NAME})


# Uninstall

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
  @ONLY
)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
