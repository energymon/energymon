# Libraries

add_library(energymon-dummy SHARED em-dummy.c)
SET_FPIC(energymon-dummy)

add_library(energymon-dummy-static STATIC em-dummy.c)
SET_FPIC(energymon-dummy-static)

if(DEFAULT STREQUAL "dummy")
  add_library(energymon-default SHARED em-dummy.c)
  SET_FPIC(energymon-default)

  add_library(energymon-default-static STATIC em-dummy.c)
  SET_FPIC(energymon-default-static)

  set_target_properties(energymon-default energymon-default-static PROPERTIES COMPILE_DEFINITIONS "ENERGYMON_DEFAULT")
endif()

# pkg-config

set(PKG_CONFIG_NAME "energymon-dummy")
set(PKG_CONFIG_DESCRIPTION "Dummy EnergyMon implementation")
set(PKG_CONFIG_LIBS "-L\${libdir} -lenergymon-dummy")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/../pkgconfig.in"
  "${CMAKE_BINARY_DIR}/pkgconfig/energymon-dummy.pc"
)
set(PKG_CONFIG_NAME "energymon-dummy-static")
set(PKG_CONFIG_LIBS "-L\${libdir} -lenergymon-dummy-static")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/../pkgconfig.in"
  "${CMAKE_BINARY_DIR}/pkgconfig/energymon-dummy-static.pc"
)
