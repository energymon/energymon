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

  add_library(${LNAME} ${SOURCES})
  target_include_directories(${LNAME} PRIVATE ${PROJECT_SOURCE_DIR}/inc ${PROJECT_SOURCE_DIR}/common)
  if(BUILD_SHARED_LIBS)
    set_target_properties(${LNAME} PROPERTIES VERSION ${PROJECT_VERSION}
                                              SOVERSION ${PROJECT_VERSION_MAJOR})
  endif()
  PKG_CONFIG("${LNAME}" "${DESCRIPTION}" "" "")
  install(TARGETS ${LNAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
  install(FILES ${LNAME}.h
                ${LNAME}-energy.h
                ${LNAME}-accel_energy.h
                ${LNAME}-cpu_energy.h
                ${LNAME}-memory_energy.h
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

endif()

# We have to set preprocessor definitions other than "ENERGYMON_BUILD_DEFAULT" when building default impl b/c we compile multiple impls in the library
if(ENERGYMON_BUILD_DEFAULT STREQUAL "cray-pm" OR ENERGYMON_BUILD_DEFAULT STREQUAL LNAME)
  BUILD_DEFAULT("${SOURCES}" "" "" "" "ENERGYMON_BUILD_DEFAULT_CRAY_PM")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "" "")
elseif(ENERGYMON_BUILD_DEFAULT STREQUAL "cray-pm-energy" OR ENERGYMON_BUILD_DEFAULT STREQUAL "energymon-cray-pm-energy")
  BUILD_DEFAULT("${SOURCES}" "" "" "" "ENERGYMON_BUILD_DEFAULT_CRAY_PM_ENERGY")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "" "")
elseif(ENERGYMON_BUILD_DEFAULT STREQUAL "cray-pm-accel_energy" OR ENERGYMON_BUILD_DEFAULT STREQUAL "energymon-cray-pm-accel_energy")
  BUILD_DEFAULT("${SOURCES}" "" "" "" "ENERGYMON_BUILD_DEFAULT_CRAY_PM_ACCEL_ENERGY")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "" "")
elseif(ENERGYMON_BUILD_DEFAULT STREQUAL "cray-pm-cpu_energy" OR ENERGYMON_BUILD_DEFAULT STREQUAL "energymon-cray-pm-cpu_energy")
  BUILD_DEFAULT("${SOURCES}" "" "" "" "ENERGYMON_BUILD_DEFAULT_CRAY_PM_CPU_ENERGY")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "" "")
elseif(ENERGYMON_BUILD_DEFAULT STREQUAL "cray-pm-memory_energy" OR ENERGYMON_BUILD_DEFAULT STREQUAL "energymon-cray-pm-memory_energy")
  BUILD_DEFAULT("${SOURCES}" "" "" "" "ENERGYMON_BUILD_DEFAULT_CRAY_PM_MEMORY_ENERGY")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "" "")
endif()

# Utils

CONFIGURE_ENERGYMON_GET_C(${LNAME}.h energymon_get_cray_pm ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
BUILD_UTILS(${SNAME} "" ${LNAME} ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
BUILD_TESTS(${SNAME} "" ${LNAME} ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
