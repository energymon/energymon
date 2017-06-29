cmake_minimum_required(VERSION 2.8)

project(energymon)
set(PROJECT_VERSION 0.2.0)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unknown-pragmas -Wextra -pedantic -std=gnu99")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(ENERGYMON_UTIL ${CMAKE_SOURCE_DIR}/src/energymon-util.c)
set(ENERGYMON_TIME_UTIL ${CMAKE_SOURCE_DIR}/src/energymon-time-util.c;${CMAKE_SOURCE_DIR}/src/ptime/ptime.c)

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  # Determine if we should link with librt for libraries that use "clock_gettime"
  include(CheckFunctionExists)
  CHECK_FUNCTION_EXISTS(clock_gettime HAVE_CLOCK_GETTIME)
  if(NOT HAVE_CLOCK_GETTIME)
    find_library(LIBRT NAMES rt)
  endif()
endif()

include_directories(${PROJECT_SOURCE_DIR}/inc)
include_directories(${PROJECT_SOURCE_DIR}/src)

macro(BUILD_DEFAULT source deplibs definitions)
  add_library(energymon-default ${source})
  target_link_libraries(energymon-default ${deplibs})
  set_target_properties(energymon-default PROPERTIES COMPILE_DEFINITIONS "ENERGYMON_DEFAULT;${definitions}")
endmacro(BUILD_DEFAULT)

macro(PKG_CONFIG name description requires_private libs_private)
  set(PKG_CONFIG_PREFIX "${CMAKE_INSTALL_PREFIX}")
  set(PKG_CONFIG_EXEC_PREFIX "\${prefix}")
  set(PKG_CONFIG_LIBDIR "\${prefix}/lib")
  set(PKG_CONFIG_INCLUDEDIR "\${prefix}/include/energymon")
  set(PKG_CONFIG_NAME "${name}")
  set(PKG_CONFIG_DESCRIPTION "${description}")
  set(PKG_CONFIG_REQUIRES_PRIVATE "${requires_private}")
  set(PKG_CONFIG_CFLAGS "-I\${includedir}")
  set(PKG_CONFIG_LIBS "-L\${libdir} -l${name}")
  set(PKG_CONFIG_LIBS_PRIVATE "${libs_private}")
  # The config file that will be installed
  configure_file(
    ${CMAKE_SOURCE_DIR}/pkgconfig.in
    ${CMAKE_BINARY_DIR}/pkgconfig/${name}.pc
    @ONLY
  )
  install(FILES ${CMAKE_BINARY_DIR}/pkgconfig/${name}.pc DESTINATION lib/pkgconfig)
  # The config file that points to the local build (without install)
  set(PKG_CONFIG_PREFIX "${CMAKE_BINARY_DIR}")
  set(PKG_CONFIG_INCLUDEDIR "${CMAKE_CURRENT_SOURCE_DIR}")
  set(PKG_CONFIG_CFLAGS "-I${CMAKE_SOURCE_DIR}/inc -I\${includedir}")
  configure_file(
    ${CMAKE_SOURCE_DIR}/pkgconfig.in
    ${CMAKE_BINARY_DIR}/${name}.pc
    @ONLY
  )
endmacro(PKG_CONFIG)

# Libraries

set(DEFAULT "dummy" CACHE STRING "Default EnergyMon implementation")

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

# Binaries

add_executable(energymon-info src/app/energymon-info.c)
target_link_libraries(energymon-info energymon-default)

if(UNIX)
  add_executable(energymon-cmd-profile src/app/energymon-cmd-profile.c;${ENERGYMON_TIME_UTIL})
  target_link_libraries(energymon-cmd-profile energymon-default ${LIBRT})

  add_executable(energymon-file-provider src/app/energymon-file-provider.c;${ENERGYMON_TIME_UTIL})
  target_link_libraries(energymon-file-provider energymon-default ${LIBRT})

  add_executable(energymon-idle-power src/app/energymon-idle-power.c;${ENERGYMON_TIME_UTIL})
  target_link_libraries(energymon-idle-power energymon-default ${LIBRT})

  add_executable(energymon-overhead src/app/energymon-overhead.c;${ENERGYMON_TIME_UTIL})
  target_link_libraries(energymon-overhead energymon-default ${LIBRT})

  add_executable(energymon-power-poller src/app/energymon-power-poller.c;${ENERGYMON_TIME_UTIL})
  target_link_libraries(energymon-power-poller energymon-default ${LIBRT} m)

  install(TARGETS energymon-cmd-profile energymon-file-provider energymon-idle-power energymon-overhead energymon-power-poller DESTINATION bin)
endif()

# Tests

add_executable(interface-test test/interface_test.c)
target_link_libraries(interface-test energymon-default)

add_executable(interval-test test/interval_test.c)
target_link_libraries(interval-test energymon-default)

# Install

install(TARGETS energymon-info DESTINATION bin)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/inc/ DESTINATION include/${PROJECT_NAME})

# Uninstall

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
  @ONLY
)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
