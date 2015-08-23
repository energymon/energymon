set(LNAME energymon-osp)
set(SOURCES ${LNAME}.c;../src/energymon-util.c)

# Dependencies

find_package(PkgConfig)
pkg_search_module(HIDAPIUSB hidapi-libusb)
if(NOT ${HIDAPIUSB_FOUND})
  message(ERROR "${LNAME}: Failed to find libhidapi-libusb - skipping this project")
  return()
endif(NOT ${HIDAPIUSB_FOUND})

# Libraries

add_library(${LNAME} SHARED ${SOURCES})
target_link_libraries(${LNAME} hidapi-libusb)
SET_FPIC(${LNAME})

add_library(${LNAME}-static STATIC ${SOURCES})
target_link_libraries(${LNAME}-static hidapi-libusb)
SET_FPIC(${LNAME}-static)

add_library(${LNAME}-polling SHARED ${SOURCES})
target_link_libraries(${LNAME}-polling hidapi-libusb pthread)
SET_FPIC(${LNAME}-polling)

add_library(${LNAME}-polling-static STATIC ${SOURCES})
target_link_libraries(${LNAME}-polling-static hidapi-libusb pthread)
SET_FPIC(${LNAME}-polling-static)

if(DEFAULT STREQUAL "osp")
  BUILD_DEFAULT("${SOURCES}" "hidapi-libusb" "")
elseif(DEFAULT STREQUAL "osp-polling")
  BUILD_DEFAULT("${SOURCES}" "hidapi-libusb;pthread" "ENERGYMON_OSP_USE_POLLING")
endif()

set_target_properties (${LNAME}-polling ${LNAME}-polling-static PROPERTIES COMPILE_DEFINITIONS "ENERGYMON_OSP_USE_POLLING")

# pkg-config

PKG_CONFIG("${LNAME}" "EnergyMon implementation for ODROID Smart Power" "-lhidapi-libusb")
PKG_CONFIG("${LNAME}-static" "EnergyMon implementation for ODROID Smart Power" "-lhidapi-libusb")

PKG_CONFIG("${LNAME}-polling" "EnergyMon implementation for ODROID Smart Power with Polling" "-lhidapi-libusb -lpthread")
PKG_CONFIG("${LNAME}-polling-static" "EnergyMon implementation for ODROID Smart Power with Polling" "-lhidapi-libusb -lpthread")

# Install

install(TARGETS ${LNAME} ${LNAME}-static DESTINATION lib)
install(TARGETS ${LNAME}-polling ${LNAME}-polling-static DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME}.h DESTINATION include/${PROJECT_NAME})
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/ DESTINATION lib/pkgconfig)
