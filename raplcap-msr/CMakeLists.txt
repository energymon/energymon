set(SNAME raplcap-msr)
set(LNAME energymon-raplcap-msr)
set(SOURCES ${LNAME}.c;${ENERGYMON_UTIL})
set(DESCRIPTION "EnergyMon implementation using libraplcap-msr")

# Dependencies

find_package(PkgConfig)
if(${PKG_CONFIG_FOUND})
  pkg_search_module(RAPLCAP IMPORTED_TARGET raplcap-msr>=0.5.0)
endif()
if(NOT RAPLCAP_FOUND)
  # fail gracefully
  message(WARNING "${LNAME}: Missing raplcap-msr - skipping this project")
  return()
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
                        ENERGYMON_GET_FUNCTION "energymon_get_raplcap_msr"
                        ENERGYMON_GET_C_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
  target_link_libraries(${LNAME} PRIVATE PkgConfig::RAPLCAP)
  add_energymon_pkg_config(${LNAME} "${DESCRIPTION}" "raplcap-msr" "")

endif()

if(ENERGYMON_BUILD_DEFAULT STREQUAL SNAME OR ENERGYMON_BUILD_DEFAULT STREQUAL LNAME)
  add_energymon_default_library(SOURCES ${SOURCES})
  target_link_libraries(energymon-default PRIVATE PkgConfig::RAPLCAP)
  add_energymon_pkg_config(energymon-default "${DESCRIPTION}" "raplcap-msr" "")
endif()
