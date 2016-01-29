set(LNAME energymon-osp)
set(SOURCES ${LNAME}.c;../src/energymon-util.c)
set(SOURCES_POLLING ${SOURCES};../src/energymon-time-util.c)
set(DESCRIPTION "EnergyMon implementation for ODROID Smart Power")
set(DESCRIPTION_POLLING "EnergyMon implementation for ODROID Smart Power with Polling")

# Dependencies

find_package(PkgConfig)
if(${PKG_CONFIG_FOUND})
  pkg_search_module(HIDAPIUSB hidapi-libusb)
endif()
if(NOT HIDAPIUSB_FOUND) # doesn't seem to work if we use ${} syntax...
  # fail gracefully
  message(WARNING "${LNAME}: Missing libhidapi-libusb - skipping this project")
  return()
endif()

include_directories(${HIDAPIUSB_INCLUDE_DIRS})

# Libraries

add_library(${LNAME} ${SOURCES})
target_link_libraries(${LNAME} -L${HIDAPIUSB_LIBDIR} ${HIDAPIUSB_LIBRARIES})

add_library(${LNAME}-polling ${SOURCES_POLLING})
target_link_libraries(${LNAME}-polling -L${HIDAPIUSB_LIBDIR} ${HIDAPIUSB_LIBRARIES} pthread rt)

if(DEFAULT STREQUAL "osp" OR DEFAULT STREQUAL "energymon-osp")
  BUILD_DEFAULT("${SOURCES}" "-L${HIDAPIUSB_LIBDIR};${HIDAPIUSB_LIBRARIES}" "")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "${HIDAPIUSB_LIBRARIES}" "")
  install(TARGETS energymon-default DESTINATION lib)
elseif(DEFAULT STREQUAL "osp-polling" OR DEFAULT STREQUAL "energymon-osp-polling")
  BUILD_DEFAULT("${SOURCES_POLLING}" "-L${HIDAPIUSB_LIBDIR};${HIDAPIUSB_LIBRARIES};pthread;rt" "ENERGYMON_OSP_USE_POLLING")
  PKG_CONFIG("energymon-default" "${DESCRIPTION_POLLING}" "${HIDAPIUSB_LIBRARIES}" "-lpthread -lrt")
  install(TARGETS energymon-default DESTINATION lib)
endif()

set_target_properties(${LNAME}-polling PROPERTIES COMPILE_DEFINITIONS "ENERGYMON_OSP_USE_POLLING")

# pkg-config

PKG_CONFIG("${LNAME}" "${DESCRIPTION}" "${HIDAPIUSB_LIBRARIES}" "")

PKG_CONFIG("${LNAME}-polling" "${DESCRIPTION_POLLING}" "${HIDAPIUSB_LIBRARIES}" "-lpthread -lrt")

# Install

install(TARGETS ${LNAME} DESTINATION lib)
install(TARGETS ${LNAME}-polling DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME}.h DESTINATION include/${PROJECT_NAME})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME}-polling.h DESTINATION include/${PROJECT_NAME})
