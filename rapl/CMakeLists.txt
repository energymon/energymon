# Libraries

add_library(energymon-rapl SHARED em-rapl.c)
SET_FPIC(energymon-rapl)

add_library(energymon-rapl-static STATIC em-rapl.c)
SET_FPIC(energymon-rapl-static)

if(DEFAULT STREQUAL "rapl")
  BUILD_DEFAULT("em-rapl.c" "" "")
endif()

# pkg-config

PKG_CONFIG("energymon-rapl" "EnergyMon implementation for Intel RAPL" "")
PKG_CONFIG("energymon-rapl-static" "EnergyMon implementation for Intel RAPL" "")
