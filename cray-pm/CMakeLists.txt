set(SNAME cray-pm)
set(LNAME energymon-cray-pm)
set(SOURCES ${LNAME}.c;
            energymon-cray-pm-energy.c;
            energymon-cray-pm-accel_energy.c;
            energymon-cray-pm-cpu_energy.c;
            energymon-cray-pm-memory_energy.c;
            energymon-cray-pm-common.c;
            ${ENERGYMON_UTIL})
set(DESCRIPTION "EnergyMon implementations for Cray Power Monitoring")

# Libraries

if(ENERGYMON_BUILD_LIB STREQUAL "ALL" OR
   ENERGYMON_BUILD_LIB STREQUAL SNAME OR
   ENERGYMON_BUILD_LIB STREQUAL LNAME)

  add_energymon_library(${LNAME} ${SNAME}
                        SOURCES ${SOURCES}
                        PUBLIC_HEADER ${LNAME}.h ${LNAME}-energy.h ${LNAME}-accel_energy.h ${LNAME}-cpu_energy.h ${LNAME}-memory_energy.h
                        PUBLIC_BUILD_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                        ENERGYMON_GET_HEADER ${LNAME}.h
                        ENERGYMON_GET_FUNCTION "energymon_get_cray_pm"
                        ENERGYMON_GET_C_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
  add_energymon_pkg_config(${LNAME} "${DESCRIPTION}" "" "")

endif()

# We have to set preprocessor definitions other than "ENERGYMON_DEFAULT" when building default impl b/c we compile multiple impls in the library
if(ENERGYMON_BUILD_DEFAULT STREQUAL "cray-pm" OR ENERGYMON_BUILD_DEFAULT STREQUAL LNAME)
  add_energymon_default_library(SOURCES ${SOURCES})
  target_compile_definitions(energymon-default PRIVATE "ENERGYMON_DEFAULT_CRAY_PM")
  add_energymon_pkg_config(energymon-default "${DESCRIPTION}" "" "")
elseif(ENERGYMON_BUILD_DEFAULT STREQUAL "cray-pm-energy" OR ENERGYMON_BUILD_DEFAULT STREQUAL "energymon-cray-pm-energy")
  add_energymon_default_library(SOURCES ${SOURCES})
  target_compile_definitions(energymon-default PRIVATE "ENERGYMON_DEFAULT_CRAY_PM_ENERGY")
  add_energymon_pkg_config(energymon-default "${DESCRIPTION}" "" "")
elseif(ENERGYMON_BUILD_DEFAULT STREQUAL "cray-pm-accel_energy" OR ENERGYMON_BUILD_DEFAULT STREQUAL "energymon-cray-pm-accel_energy")
  add_energymon_default_library(SOURCES ${SOURCES})
  target_compile_definitions(energymon-default PRIVATE "ENERGYMON_DEFAULT_CRAY_PM_ACCEL_ENERGY")
  add_energymon_pkg_config(energymon-default "${DESCRIPTION}" "" "")
elseif(ENERGYMON_BUILD_DEFAULT STREQUAL "cray-pm-cpu_energy" OR ENERGYMON_BUILD_DEFAULT STREQUAL "energymon-cray-pm-cpu_energy")
  add_energymon_default_library(SOURCES ${SOURCES})
  target_compile_definitions(energymon-default PRIVATE "ENERGYMON_DEFAULT_CRAY_PM_CPU_ENERGY")
  add_energymon_pkg_config(energymon-default "${DESCRIPTION}" "" "")
elseif(ENERGYMON_BUILD_DEFAULT STREQUAL "cray-pm-memory_energy" OR ENERGYMON_BUILD_DEFAULT STREQUAL "energymon-cray-pm-memory_energy")
  add_energymon_default_library(SOURCES ${SOURCES})
  target_compile_definitions(energymon-default PRIVATE "ENERGYMON_DEFAULT_CRAY_PM_MEMORY_ENERGY")
  add_energymon_pkg_config(energymon-default "${DESCRIPTION}" "" "")
endif()
