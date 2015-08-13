# Libraries

add_library(energymon-odroid SHARED em-odroid.c)
target_link_libraries(energymon-odroid pthread)
SET_FPIC(energymon-odroid)

add_library(energymon-odroid-static STATIC em-odroid.c)
target_link_libraries(energymon-odroid-static pthread)
SET_FPIC(energymon-odroid-static)

if(DEFAULT STREQUAL "odroid")
  BUILD_DEFAULT("em-odroid.c" "pthread" "")
endif()

# pkg-config

PKG_CONFIG("energymon-odroid" "EnergyMon implementation for ODROID systems" "-lpthread")
PKG_CONFIG("energymon-odroid-static" "EnergyMon implementation for ODROID systems" "-lpthread")
