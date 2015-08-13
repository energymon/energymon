# Libraries

add_library(energymon-msr SHARED em-msr.c)
target_link_libraries(energymon-msr m)
SET_FPIC(energymon-msr)

add_library(energymon-msr-static STATIC em-msr.c)
target_link_libraries(energymon-msr-static m)
SET_FPIC(energymon-msr-static)

if(DEFAULT STREQUAL "msr")
  add_library(energymon-default SHARED em-msr.c)
  target_link_libraries(energymon-default m)
  SET_FPIC(energymon-default)

  add_library(energymon-default-static STATIC em-msr.c)
  target_link_libraries(energymon-default-static m)
  SET_FPIC(energymon-default-static)

  set_target_properties(energymon-default energymon-default-static PROPERTIES COMPILE_DEFINITIONS "ENERGYMON_DEFAULT")
endif()

# pkg-config

set(PKG_CONFIG_NAME "energymon-msr")
set(PKG_CONFIG_DESCRIPTION "EnergyMon implementation for Intel Model Specific Register")
set(PKG_CONFIG_LIBS "-L\${libdir} -lenergymon-msr -lm")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/../pkgconfig.in"
  "${CMAKE_BINARY_DIR}/pkgconfig/energymon-msr.pc"
)
set(PKG_CONFIG_NAME "energymon-msr-static")
set(PKG_CONFIG_LIBS "-L\${libdir} -lenergymon-msr-static -lm")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/../pkgconfig.in"
  "${CMAKE_BINARY_DIR}/pkgconfig/energymon-msr-static.pc"
)
