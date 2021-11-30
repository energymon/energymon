set(SNAME ipg)
set(LNAME energymon-ipg)
set(SOURCES ${LNAME}.c;${ENERGYMON_UTIL})
set(DESCRIPTION "Intel Power Gadget EnergyMon implementation")

# Dependencies

set(IPG_MIN_VERSION 3.7)
find_package(IntelPowerGadget ${IPG_MIN_VERSION})
if(NOT IntelPowerGadget_FOUND)
  message(WARNING "${LNAME}: Missing Intel Power Gadget - skipping this project")
  return()
endif()

if(APPLE)
  set(PKG_CONFIG_PRIVATE_LIBS "-framework IntelPowerGadget")
endif()

# Libraries

if(ENERGYMON_BUILD_LIB STREQUAL "ALL" OR
   ENERGYMON_BUILD_LIB STREQUAL SNAME OR
   ENERGYMON_BUILD_LIB STREQUAL LNAME)

  add_energymon_library(${LNAME} ${SNAME}
                        SOURCES ${SOURCES}
                        PUBLIC_HEADER ${LNAME}.h
                        PUBLIC_BUILD_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                        ENERGYMON_GET_HEADER ${LNAME}.h
                        ENERGYMON_GET_FUNCTION "energymon_get_ipg"
                        ENERGYMON_GET_C_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
  target_link_libraries(${LNAME} PRIVATE IntelPowerGadget::IntelPowerGadget)
  add_energymon_pkg_config(${LNAME} "${DESCRIPTION}" "" "${PKG_CONFIG_PRIVATE_LIBS}")
  energymon_export_dependency("IntelPowerGadget ${IPG_MIN_VERSION}" IMPORTED_TARGET)

endif()

if(ENERGYMON_BUILD_DEFAULT STREQUAL SNAME OR ENERGYMON_BUILD_DEFAULT STREQUAL LNAME)
  add_energymon_default_library(SOURCES ${SOURCES})
  target_link_libraries(energymon-default PRIVATE IntelPowerGadget::IntelPowerGadget)
  add_energymon_pkg_config(energymon-default "${DESCRIPTION}" "" "${PKG_CONFIG_PRIVATE_LIBS}")
  energymon_export_dependency("IntelPowerGadget ${IPG_MIN_VERSION}" IMPORTED_TARGET)
endif()
