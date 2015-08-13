# Libraries

add_library(energymon-dummy SHARED em-dummy.c)
SET_FPIC(energymon-dummy)

add_library(energymon-dummy-static STATIC em-dummy.c)
SET_FPIC(energymon-dummy-static)

if(DEFAULT STREQUAL "dummy")
  BUILD_DEFAULT("em-dummy.c" "" "")
endif()

# pkg-config

PKG_CONFIG("energymon-dummy" "Dummy EnergyMon implementation" "")
PKG_CONFIG("energymon-dummy-static" "Dummy EnergyMon implementation" "")
