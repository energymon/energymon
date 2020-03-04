cmake_minimum_required(VERSION 2.8.5)

project(energymon)
set(VERSION_MAJOR 0)
set(VERSION_MINOR 2)
set(VERSION_PATCH 4)
set(PROJECT_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

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

if(CMAKE_HOST_UNIX)
  include(GNUInstallDirs)
else()
  set(CMAKE_INSTALL_LIBDIR lib)
  set(CMAKE_INSTALL_INCLUDEDIR include)
  set(CMAKE_INSTALL_BINDIR bin)
  set(CMAKE_INSTALL_MANDIR share/man)
endif()

include_directories(${PROJECT_SOURCE_DIR}/inc)
include_directories(${PROJECT_SOURCE_DIR}/common)


# Utilities

add_subdirectory(utils)
add_subdirectory(test)


# Macros

macro(CONFIGURE_ENERGYMON_GET_C get_header get_function output)
  set(ENERGYMON_GET_HEADER ${get_header})
  set(ENERGYMON_GET_FUNCTION ${get_function})
  configure_file(${CMAKE_SOURCE_DIR}/common/energymon-get.c.in ${output})
endmacro(CONFIGURE_ENERGYMON_GET_C)

macro(BUILD_DEFAULT source deplibs definitions)
  add_library(energymon-default ${source})
  target_link_libraries(energymon-default ${deplibs})
  if(BUILD_SHARED_LIBS)
    set_target_properties(energymon-default PROPERTIES VERSION ${PROJECT_VERSION}
                                                       SOVERSION ${VERSION_MAJOR})
  endif()
  set_target_properties(energymon-default PROPERTIES COMPILE_DEFINITIONS "ENERGYMON_DEFAULT;${definitions}")
  install(TARGETS energymon-default DESTINATION ${CMAKE_INSTALL_LIBDIR})
  install(FILES ${CMAKE_SOURCE_DIR}/inc/energymon-default.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
  CONFIGURE_ENERGYMON_GET_C(energymon-default.h energymon_get_default ${CMAKE_CURRENT_BINARY_DIR}/energymon-default/energymon-get.c)
  BUILD_UTILS(energymon energymon-default ${CMAKE_CURRENT_BINARY_DIR}/energymon-default/energymon-get.c)
  BUILD_TESTS(energymon energymon-default ${CMAKE_CURRENT_BINARY_DIR}/energymon-default/energymon-get.c)
endmacro(BUILD_DEFAULT)

macro(PKG_CONFIG name description requires_private libs_private)
  set(PKG_CONFIG_PREFIX "${CMAKE_INSTALL_PREFIX}")
  set(PKG_CONFIG_EXEC_PREFIX "\${prefix}")
  set(PKG_CONFIG_LIBDIR "\${prefix}/${CMAKE_INSTALL_LIBDIR}")
  set(PKG_CONFIG_INCLUDEDIR "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}")
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
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${name}.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
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
  add_subdirectory(raplcap-msr)
  add_subdirectory(zcu102)
endif()

if(UNIX OR WIN32)
  add_subdirectory(osp)
  add_subdirectory(wattsup)
endif()

if(NOT TARGET energymon-default AND NOT ${ENERGYMON_BUILD_DEFAULT} MATCHES "NONE")
  message(FATAL_ERROR
          "No build target for ENERGYMON_BUILD_DEFAULT=${ENERGYMON_BUILD_DEFAULT}\n"
          "Specified implementation may be unknown, unsupported for the target system, or skipped due to missing build dependencies\n"
          "Retry with a supported value or set ENERGYMON_BUILD_DEFAULT=NONE")
endif()


# Install

install(FILES inc/energymon.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})


# Uninstall

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
  @ONLY
)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
