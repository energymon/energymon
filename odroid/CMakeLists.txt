# Libraries

add_library(energymon-odroid SHARED em-odroid.c)
target_link_libraries(energymon-odroid pthread)
SET_FPIC(energymon-odroid)

add_library(energymon-odroid-static STATIC em-odroid.c)
target_link_libraries(energymon-odroid-static pthread)
SET_FPIC(energymon-odroid-static)

if(DEFAULT STREQUAL "odroid")
  add_library(energymon-default SHARED em-odroid.c)
  target_link_libraries(energymon-default pthread)
  SET_FPIC(energymon-default)

  add_library(energymon-default-static STATIC em-odroid.c)
  target_link_libraries(energymon-default-static pthread)
  SET_FPIC(energymon-default-static)

  set_target_properties(energymon-default energymon-default-static PROPERTIES COMPILE_DEFINITIONS "ENERGYMON_DEFAULT")
endif()

# pkg-config

set(PKG_CONFIG_NAME "energymon-odroid")
set(PKG_CONFIG_DESCRIPTION "EnergyMon implementation for ODROID systems")
set(PKG_CONFIG_LIBS "-L\${libdir} -lenergymon-odroid -lpthread")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/../pkgconfig.in"
  "${CMAKE_BINARY_DIR}/pkgconfig/energymon-odroid.pc"
)
set(PKG_CONFIG_NAME "energymon-odroid-static")
set(PKG_CONFIG_LIBS "-L\${libdir} -lenergymon-odroid-static -lpthread")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/../pkgconfig.in"
  "${CMAKE_BINARY_DIR}/pkgconfig/energymon-odroid-static.pc"
)
