# Libraries

add_library(energymon-rapl SHARED em-rapl.c)
SET_FPIC(energymon-rapl)

add_library(energymon-rapl-static STATIC em-rapl.c)
SET_FPIC(energymon-rapl-static)

if(DEFAULT STREQUAL "rapl")
  add_library(energymon-default SHARED em-rapl.c)
  SET_FPIC(energymon-default)

  add_library(energymon-default-static STATIC em-rapl.c)
  SET_FPIC(energymon-default-static)

  set_target_properties(energymon-default energymon-default-static PROPERTIES COMPILE_DEFINITIONS "ENERGYMON_DEFAULT")
endif()

# pkg-config

set(PKG_CONFIG_NAME "energymon-rapl")
set(PKG_CONFIG_DESCRIPTION "EnergyMon implementation for Intel RAPL")
set(PKG_CONFIG_LIBS "-L\${libdir} -lenergymon-rapl")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/../pkgconfig.in"
  "${CMAKE_BINARY_DIR}/pkgconfig/energymon-rapl.pc"
)
set(PKG_CONFIG_NAME "energymon-rapl-static")
set(PKG_CONFIG_LIBS "-L\${libdir} -lenergymon-rapl-static")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/../pkgconfig.in"
  "${CMAKE_BINARY_DIR}/pkgconfig/energymon-rapl-static.pc"
)
