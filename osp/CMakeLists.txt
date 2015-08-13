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
  add_library(energymon-default SHARED em-osp.c)
  target_link_libraries(energymon-default hidapi-libusb)
  SET_FPIC(energymon-default)

  add_library(energymon-default-static STATIC em-osp.c)
  target_link_libraries(energymon-default-static hidapi-libusb)
  SET_FPIC(energymon-default-static)

  set_target_properties(energymon-default energymon-default-static PROPERTIES COMPILE_DEFINITIONS "ENERGYMON_DEFAULT")
elseif(DEFAULT STREQUAL "osp-polling")
  add_library(energymon-default SHARED em-osp.c)
  target_link_libraries(energymon-default hidapi-libusb pthread)
  SET_FPIC(energymon-default)

  add_library(energymon-default-static STATIC em-osp.c)
  target_link_libraries(energymon-default-static hidapi-libusb pthread)
  SET_FPIC(energymon-default-static)

  set_target_properties(energymon-default energymon-default-static PROPERTIES COMPILE_DEFINITIONS "ENERGYMON_DEFAULT;ENERGYMON_OSP_USE_POLLING")
endif()

set_target_properties (energymon-osp-polling energymon-osp-polling-static PROPERTIES COMPILE_DEFINITIONS "ENERGYMON_OSP_USE_POLLING")

# pkg-config

set(PKG_CONFIG_NAME "energymon-osp")
set(PKG_CONFIG_DESCRIPTION "EnergyMon implementation for ODROID Smart Power")
set(PKG_CONFIG_LIBS "-L\${libdir} -lenergymon-osp -lhidapi-libusb")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/../pkgconfig.in"
  "${CMAKE_BINARY_DIR}/pkgconfig/energymon-osp.pc"
)
set(PKG_CONFIG_NAME "energymon-osp-static")
set(PKG_CONFIG_LIBS "-L\${libdir} -lenergymon-osp-static -lhidapi-libusb")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/../pkgconfig.in"
  "${CMAKE_BINARY_DIR}/pkgconfig/energymon-osp-static.pc"
)

set(PKG_CONFIG_NAME "energymon-osp-polling")
set(PKG_CONFIG_DESCRIPTION "EnergyMon implementation for ODROID Smart Power with Polling")
set(PKG_CONFIG_LIBS "-L\${libdir} -lenergymon-osp-polling -lhidapi-libusb -lpthread")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/../pkgconfig.in"
  "${CMAKE_BINARY_DIR}/pkgconfig/energymon-osp-polling.pc"
)
set(PKG_CONFIG_NAME "energymon-osp-polling-static")
set(PKG_CONFIG_LIBS "-L\${libdir} -lenergymon-osp-polling-static -lhidapi-libusb -lpthread")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/../pkgconfig.in"
  "${CMAKE_BINARY_DIR}/pkgconfig/energymon-osp-polling-static.pc"
)
