set(LNAME energymon-osp)
set(SOURCES ${LNAME}.c;../src/energymon-util.c)
set(SOURCES_POLLING ${SOURCES};../src/energymon-time-util.c)
set(DESCRIPTION "EnergyMon implementation for ODROID Smart Power")
set(DESCRIPTION_POLLING "EnergyMon implementation for ODROID Smart Power with Polling")

# Dependencies

find_package(PkgConfig REQUIRED)
pkg_search_module(HIDAPIUSB hidapi-libusb)
if(NOT ${HIDAPIUSB_FOUND})
  message(ERROR "${LNAME}: Failed to find libhidapi-libusb - skipping this project")
  return()
endif(NOT ${HIDAPIUSB_FOUND})

# Libraries

add_library(${LNAME} SHARED ${SOURCES})
target_link_libraries(${LNAME} hidapi-libusb)

add_library(${LNAME}-static STATIC ${SOURCES})
target_link_libraries(${LNAME}-static hidapi-libusb)

add_library(${LNAME}-polling SHARED ${SOURCES_POLLING})
target_link_libraries(${LNAME}-polling hidapi-libusb pthread rt)

add_library(${LNAME}-polling-static STATIC ${SOURCES_POLLING})
target_link_libraries(${LNAME}-polling-static hidapi-libusb pthread rt)

if(DEFAULT STREQUAL "osp")
  BUILD_DEFAULT("${SOURCES}" "hidapi-libusb" "")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "-lhidapi-libusb")
  PKG_CONFIG("energymon-default-static" "${DESCRIPTION}" "-lhidapi-libusb")
  install(TARGETS energymon-default energymon-default-static DESTINATION lib)
elseif(DEFAULT STREQUAL "osp-polling")
  BUILD_DEFAULT("${SOURCES_POLLING}" "hidapi-libusb;pthread;rt" "ENERGYMON_OSP_USE_POLLING")
  PKG_CONFIG("energymon-default" "${DESCRIPTION_POLLING}" "-lhidapi-libusb -lpthread -lrt")
  PKG_CONFIG("energymon-default-static" "${DESCRIPTION_POLLING}" "-lhidapi-libusb -lpthread -lrt")
  install(TARGETS energymon-default energymon-default-static DESTINATION lib)
endif()

set_target_properties (${LNAME}-polling ${LNAME}-polling-static PROPERTIES COMPILE_DEFINITIONS "ENERGYMON_OSP_USE_POLLING")

# pkg-config

PKG_CONFIG("${LNAME}" "${DESCRIPTION}" "-lhidapi-libusb")
PKG_CONFIG("${LNAME}-static" "${DESCRIPTION}" "-lhidapi-libusb")

PKG_CONFIG("${LNAME}-polling" "${DESCRIPTION_POLLING}" "-lhidapi-libusb -lpthread -lrt")
PKG_CONFIG("${LNAME}-polling-static" "${DESCRIPTION_POLLING}" "-lhidapi-libusb -lpthread -lrt")

# Install

install(TARGETS ${LNAME} ${LNAME}-static DESTINATION lib)
install(TARGETS ${LNAME}-polling ${LNAME}-polling-static DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME}.h DESTINATION include/${PROJECT_NAME})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME}-polling.h DESTINATION include/${PROJECT_NAME})
