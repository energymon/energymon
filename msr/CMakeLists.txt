# Libraries

add_library(energymon-msr SHARED em-msr.c)
target_link_libraries(energymon-msr m)
SET_FPIC(energymon-msr)

add_library(energymon-msr-static STATIC em-msr.c)
target_link_libraries(energymon-msr-static m)
SET_FPIC(energymon-msr-static)

if(DEFAULT STREQUAL "msr")
  BUILD_DEFAULT("em-msr.c" "m" "")
endif()

# pkg-config

PKG_CONFIG("energymon-msr" "EnergyMon implementation for Intel Model Specific Register" "-lm")
PKG_CONFIG("energymon-msr-static" "EnergyMon implementation for Intel Model Specific Register" "-lm")
