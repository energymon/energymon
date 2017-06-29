set(LNAME energymon-cray-pm)
set(SOURCES ${LNAME}.c;
            energymon-cray-pm-energy.c;
            energymon-cray-pm-accel_energy.c;
            energymon-cray-pm-cpu_energy.c;
            energymon-cray-pm-memory_energy.c;
            energymon-cray-pm-common.c;
            ../src/energymon-util.c)
set(DESCRIPTION "EnergyMon implementations for Cray Power Monitoring")

# Libraries

add_library(${LNAME} ${SOURCES})

# We have to set preprocessor definitions other than "ENERGYMON_DEFAULT" when building default impl b/c we compile multiple impls in the library
if(DEFAULT STREQUAL "cray-pm" OR DEFAULT STREQUAL LNAME)
  BUILD_DEFAULT("${SOURCES}" "" "ENERGYMON_DEFAULT_CRAY_PM")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "" "")
  install(TARGETS energymon-default DESTINATION lib)
elseif(DEFAULT STREQUAL "cray-pm-energy" OR DEFAULT STREQUAL "energymon-cray-pm-energy")
  BUILD_DEFAULT("${SOURCES}" "" "ENERGYMON_DEFAULT_CRAY_PM_ENERGY")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "" "")
  install(TARGETS energymon-default DESTINATION lib)
elseif(DEFAULT STREQUAL "cray-pm-accel_energy" OR DEFAULT STREQUAL "energymon-cray-pm-accel_energy")
  BUILD_DEFAULT("${SOURCES}" "" "ENERGYMON_DEFAULT_CRAY_PM_ACCEL_ENERGY")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "" "")
  install(TARGETS energymon-default DESTINATION lib)
elseif(DEFAULT STREQUAL "cray-pm-cpu_energy" OR DEFAULT STREQUAL "energymon-cray-pm-cpu_energy")
  BUILD_DEFAULT("${SOURCES}" "" "ENERGYMON_DEFAULT_CRAY_PM_CPU_ENERGY")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "" "")
  install(TARGETS energymon-default DESTINATION lib)
elseif(DEFAULT STREQUAL "cray-pm-memory_energy" OR DEFAULT STREQUAL "energymon-cray-pm-memory_energy")
  BUILD_DEFAULT("${SOURCES}" "" "ENERGYMON_DEFAULT_CRAY_PM_MEMORY_ENERGY")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "" "")
  install(TARGETS energymon-default DESTINATION lib)
endif()

# pkg-config

PKG_CONFIG("${LNAME}" "${DESCRIPTION}" "" "")

# Install

install(TARGETS ${LNAME} DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME}.h
              ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME}-energy.h
              ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME}-accel_energy.h
              ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME}-cpu_energy.h
              ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME}-memory_energy.h
        DESTINATION include/${PROJECT_NAME})
