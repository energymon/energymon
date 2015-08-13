# Libraries

add_library(energymon-osp SHARED em-osp.c)
target_link_libraries(energymon-osp hidapi-libusb)
SET_FPIC(energymon-osp)

add_library(energymon-osp-static STATIC em-osp.c)
target_link_libraries(energymon-osp-static hidapi-libusb)
SET_FPIC(energymon-osp-static)

add_library(energymon-osp-polling SHARED em-osp.c)
target_link_libraries(energymon-osp-polling hidapi-libusb pthread)
SET_FPIC(energymon-osp-polling)

add_library(energymon-osp-polling-static STATIC em-osp.c)
target_link_libraries(energymon-osp-polling-static hidapi-libusb pthread)
SET_FPIC(energymon-osp-polling-static)

if(DEFAULT STREQUAL "osp")
  BUILD_DEFAULT("em-osp.c" "hidapi-libusb" "")
elseif(DEFAULT STREQUAL "osp-polling")
  BUILD_DEFAULT("em-osp.c" "hidapi-libusb;pthread" "ENERGYMON_OSP_USE_POLLING")
endif()

set_target_properties (energymon-osp-polling energymon-osp-polling-static PROPERTIES COMPILE_DEFINITIONS "ENERGYMON_OSP_USE_POLLING")

# pkg-config

PKG_CONFIG("energymon-osp" "EnergyMon implementation for ODROID Smart Power" "-lhidapi-libusb")
PKG_CONFIG("energymon-osp-static" "EnergyMon implementation for ODROID Smart Power" "-lhidapi-libusb")

PKG_CONFIG("energymon-osp-polling" "EnergyMon implementation for ODROID Smart Power with Polling" "-lhidapi-libusb -lpthread")
PKG_CONFIG("energymon-osp-polling-static" "EnergyMon implementation for ODROID Smart Power with Polling" "-lhidapi-libusb -lpthread")
