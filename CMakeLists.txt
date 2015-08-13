cmake_minimum_required(VERSION 2.8)

project(energymon)
set(PROJECT_VERSION 0.1.0)

set(CMAKE_C_FLAGS "-Wall -Wno-unknown-pragmas")
set(CMAKE_BUILD_TYPE Release)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

include_directories(${PROJECT_SOURCE_DIR}/inc)

# Configure default implementation

set(DEFAULT "dummy" CACHE STRING "Default EnergyMon implementation")
if(DEFAULT STREQUAL "osp-polling" OR DEFAULT STREQUAL "osp-polling-static")
  set(DEFAULT_IMPL osp)
  set(DEFAULT_FLAGS ENERGYMON_DEFAULT;ENERGYMON_OSP_USE_POLLING)
else()
  set(DEFAULT_FLAGS ENERGYMON_DEFAULT)
  set(DEFAULT_IMPL ${DEFAULT})
endif()

set(DEFAULT_LINK_LIBS energymon-default hidapi-libusb pthread rt m)

# Libraries

add_library(energymon-dummy SHARED src/em-dummy.c)
add_library(energymon-dummy-static STATIC src/em-dummy.c)

add_library(energymon-msr SHARED src/em-msr.c)
target_link_libraries(energymon-msr m)
add_library(energymon-msr-static STATIC src/em-msr.c)
target_link_libraries(energymon-msr-static m)

add_library(energymon-odroid SHARED src/em-odroid.c)
target_link_libraries(energymon-odroid pthread)
add_library(energymon-odroid-static STATIC src/em-odroid.c)
target_link_libraries(energymon-odroid-static pthread)

add_library(energymon-osp SHARED src/em-osp.c)
target_link_libraries(energymon-osp hidapi-libusb)
add_library(energymon-osp-static STATIC src/em-osp.c)
target_link_libraries(energymon-osp-static hidapi-libusb)

add_library(energymon-osp-polling SHARED src/em-osp.c)
target_link_libraries(energymon-osp-polling hidapi-libusb pthread)
add_library(energymon-osp-polling-static STATIC src/em-osp.c)
target_link_libraries(energymon-osp-polling-static hidapi-libusb pthread)
set_target_properties (energymon-osp-polling energymon-osp-polling-static PROPERTIES COMPILE_DEFINITIONS "ENERGYMON_OSP_USE_POLLING")

add_library(energymon-rapl SHARED src/em-rapl.c)
add_library(energymon-rapl-static STATIC src/em-rapl.c)

add_library(energymon-default SHARED src/em-${DEFAULT_IMPL}.c)
target_link_libraries(energymon-default hidapi-libusb pthread rt m)
add_library(energymon-default-static STATIC src/em-${DEFAULT_IMPL}.c)
target_link_libraries(energymon-default-static hidapi-libusb pthread rt m)
set_target_properties(energymon-default energymon-default-static PROPERTIES COMPILE_DEFINITIONS "${DEFAULT_FLAGS}")

# Required to force fPIC on static libs
# 'set (CMAKE_POSITION_INDEPENDENT_CODE TRUE)' not supported until cmake 2.8.9
IF(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  SET_TARGET_PROPERTIES(energymon-dummy-static energymon-msr-static energymon-odroid-static energymon-osp-static energymon-osp-polling-static energymon-rapl-static PROPERTIES COMPILE_FLAGS "-fPIC")
ENDIF(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")

# Binaries

add_executable(energymon src/app/energymon.c)
target_link_libraries(energymon ${DEFAULT_LINK_LIBS})

# Tests

add_executable(interface-test test/interface_test.c)
target_link_libraries(interface-test ${DEFAULT_LINK_LIBS})

add_executable(interval-test test/interval_test.c)
target_link_libraries(interval-test ${DEFAULT_LINK_LIBS})

# pkg-config

set(PKG_CONFIG_EXEC_PREFIX "\${prefix}")
set(PKG_CONFIG_LIBDIR "\${prefix}/lib")
set(PKG_CONFIG_INCLUDEDIR "\${prefix}/include/energymon")
set(PKG_CONFIG_CFLAGS "-I\${includedir}")

set(PKG_CONFIG_NAME "energymon-dummy")
set(PKG_CONFIG_DESCRIPTION "Dummy EnergyMon implementation")
set(PKG_CONFIG_LIBS "-L\${libdir} -lenergymon-dummy")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig.in"
  "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/energymon-dummy.pc"
)
set(PKG_CONFIG_NAME "energymon-dummy-static")
set(PKG_CONFIG_LIBS "-L\${libdir} -lenergymon-dummy-static")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig.in"
  "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/energymon-dummy-static.pc"
)

set(PKG_CONFIG_NAME "energymon-msr")
set(PKG_CONFIG_DESCRIPTION "EnergyMon implementation for Intel Model Specific Register")
set(PKG_CONFIG_LIBS "-L\${libdir} -lenergymon-msr -lm")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig.in"
  "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/energymon-msr.pc"
)
set(PKG_CONFIG_NAME "energymon-msr-static")
set(PKG_CONFIG_LIBS "-L\${libdir} -lenergymon-msr-static -lm")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig.in"
  "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/energymon-msr-static.pc"
)

set(PKG_CONFIG_NAME "energymon-odroid")
set(PKG_CONFIG_DESCRIPTION "EnergyMon implementation for ODROID systems")
set(PKG_CONFIG_LIBS "-L\${libdir} -lenergymon-odroid -lpthread")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig.in"
  "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/energymon-odroid.pc"
)
set(PKG_CONFIG_NAME "energymon-odroid-static")
set(PKG_CONFIG_LIBS "-L\${libdir} -lenergymon-odroid-static -lpthread")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig.in"
  "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/energymon-odroid-static.pc"
)

set(PKG_CONFIG_NAME "energymon-osp")
set(PKG_CONFIG_DESCRIPTION "EnergyMon implementation for ODROID Smart Power")
set(PKG_CONFIG_LIBS "-L\${libdir} -lenergymon-osp -lhidapi-libusb")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig.in"
  "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/energymon-osp.pc"
)
set(PKG_CONFIG_NAME "energymon-osp-static")
set(PKG_CONFIG_LIBS "-L\${libdir} -lenergymon-osp-static -lhidapi-libusb")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig.in"
  "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/energymon-osp-static.pc"
)

set(PKG_CONFIG_NAME "energymon-osp-polling")
set(PKG_CONFIG_DESCRIPTION "EnergyMon implementation for ODROID Smart Power with Polling")
set(PKG_CONFIG_LIBS "-L\${libdir} -lenergymon-osp-polling -lhidapi-libusb -lpthread")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig.in"
  "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/energymon-osp-polling.pc"
)
set(PKG_CONFIG_NAME "energymon-osp-polling-static")
set(PKG_CONFIG_LIBS "-L\${libdir} -lenergymon-osp-polling-static -lhidapi-libusb -lpthread")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig.in"
  "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/energymon-osp-polling-static.pc"
)

set(PKG_CONFIG_NAME "energymon-rapl")
set(PKG_CONFIG_DESCRIPTION "EnergyMon implementation for Intel RAPL")
set(PKG_CONFIG_LIBS "-L\${libdir} -lenergymon-rapl")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig.in"
  "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/energymon-rapl.pc"
)
set(PKG_CONFIG_NAME "energymon-rapl-static")
set(PKG_CONFIG_LIBS "-L\${libdir} -lenergymon-rapl-static")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig.in"
  "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/energymon-rapl-static.pc"
)

if(DEFAULT STREQUAL "msr")
  set(PKG_CONFIG_EXTRA_LIBS "-lm")
elseif(DEFAULT STREQUAL "odroid")
  set(PKG_CONFIG_EXTRA_LIBS "-lpthread")
elseif(DEFAULT STREQUAL "osp")
  set(PKG_CONFIG_EXTRA_LIBS "-lhidapi-libusb")
elseif(DEFAULT STREQUAL "osp-polling")
  set(PKG_CONFIG_EXTRA_LIBS "-lhidapi-libusb -lpthread")
endif()
set(PKG_CONFIG_NAME "energymon-default")
set(PKG_CONFIG_DESCRIPTION "Default EnergyMon implementation")
set(PKG_CONFIG_LIBS "-L\${libdir} -lenergymon-default ${PKG_CONFIG_EXTRA_LIBS}")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig.in"
  "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/energymon-default.pc"
)
set(PKG_CONFIG_NAME "energymon-default-static")
set(PKG_CONFIG_LIBS "-L\${libdir} -lenergymon-default-static")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig.in"
  "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/energymon-default-static.pc"
)

# Install

install(TARGETS energymon-dummy energymon-msr energymon-odroid energymon-osp energymon-osp-polling energymon-default DESTINATION lib)
install(TARGETS energymon-dummy-static energymon-msr-static energymon-odroid-static energymon-osp-static energymon-osp-polling-static energymon-default-static DESTINATION lib)
install(DIRECTORY ${CMAKE_BINARY_DIR}/pkgconfig/ DESTINATION lib/pkgconfig)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/inc/ DESTINATION include/${PROJECT_NAME})
install(TARGETS energymon DESTINATION bin)

# Uninstall

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
