if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Linux|Android")
  return()
endif()

set(SNAME ibmpowernv)
set(SNAME_POWER ibmpowernv-power)
set(LNAME energymon-ibmpowernv)
set(LNAME_POWER energymon-ibmpowernv-power)
set(SOURCES ${LNAME}.c;${ENERGYMON_UTIL})
set(SOURCES_POWER ${SOURCES};${ENERGYMON_TIME_UTIL})
set(DESCRIPTION "EnergyMon implementation for IBM PowerNV system energy sensors")
set(DESCRIPTION_POWER "EnergyMon implementation for IBM PowerNV system power sensors")

# Shared Dependencies

# libsensors 3.0.1 is first version to support hwmon class directory
# Unfortunately, libsensors doesn't have its own pkg-config file, so we can't
# enforce a min dependency version in our own generated pkg-config files.
find_package(Sensors 3.0.1)
if(NOT Sensors_FOUND)
  # fail gracefully
  message(WARNING "${LNAME}: Missing libsensors - skipping this project")
  message(WARNING "${LNAME_POWER}: Missing libsensors - skipping this project")
  return()
endif()
list(APPEND PKG_CONFIG_PRIVATE_LIBS "-lsensors")
list(APPEND PKG_CONFIG_PRIVATE_LIBS_POWER "-lsensors")


# Energy Sensor Library

if(ENERGYMON_BUILD_LIB STREQUAL "ALL" OR
   ENERGYMON_BUILD_LIB STREQUAL SNAME OR
   ENERGYMON_BUILD_LIB STREQUAL LNAME)

  add_energymon_library(${LNAME} ${SNAME}
                        SOURCES ${SOURCES}
                        PUBLIC_HEADER ${LNAME}.h
                        PUBLIC_BUILD_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                        ENERGYMON_GET_HEADER ${LNAME}.h
                        ENERGYMON_GET_FUNCTION "energymon_get_ibmpowernv"
                        ENERGYMON_GET_C_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
  target_link_libraries(${LNAME} PRIVATE Sensors::Sensors)
  add_energymon_pkg_config(${LNAME} "${DESCRIPTION}" "" "${PKG_CONFIG_PRIVATE_LIBS}")
  energymon_export_dependency(Sensors)

endif()

if(ENERGYMON_BUILD_DEFAULT STREQUAL SNAME OR ENERGYMON_BUILD_DEFAULT STREQUAL LNAME)
  add_energymon_default_library(SOURCES ${SOURCES})
  target_link_libraries(energymon-default PRIVATE Sensors::Sensors)
  add_energymon_pkg_config(energymon-default "${DESCRIPTION}" "" "${PKG_CONFIG_PRIVATE_LIBS}")
  energymon_export_dependency(Sensors)
endif()


# Power Sensor Library

find_package(Threads)
if(NOT Threads_FOUND)
  # fail gracefully
  message(WARNING "${LNAME_POWER}: Missing Threads library - skipping this project")
  return()
endif()
if(CMAKE_THREAD_LIBS_INIT)
  list(APPEND PKG_CONFIG_PRIVATE_LIBS_POWER "${CMAKE_THREAD_LIBS_INIT}")
endif()

if(ENERGYMON_BUILD_LIB STREQUAL "ALL" OR
   ENERGYMON_BUILD_LIB STREQUAL SNAME_POWER OR
   ENERGYMON_BUILD_LIB STREQUAL LNAME_POWER)

  add_energymon_library(${LNAME_POWER} ${SNAME_POWER}
                        SOURCES ${SOURCES_POWER}
                        PUBLIC_HEADER ${LNAME_POWER}.h
                        PUBLIC_BUILD_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                        ENERGYMON_GET_HEADER ${LNAME_POWER}.h
                        ENERGYMON_GET_FUNCTION "energymon_get_ibmpowernv_power"
                        ENERGYMON_GET_C_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${LNAME_POWER}/energymon-get.c)
  target_compile_definitions(${LNAME_POWER} PRIVATE ENERGYMON_IBMPOWERNV_USE_POWER)
  target_link_libraries(${LNAME_POWER} PRIVATE Sensors::Sensors Threads::Threads)
  add_energymon_pkg_config(${LNAME_POWER} "${DESCRIPTION_POWER}" "" "${PKG_CONFIG_PRIVATE_LIBS_POWER}")
  energymon_export_dependency(Threads)
  energymon_export_dependency(Sensors)

endif()

if(ENERGYMON_BUILD_DEFAULT STREQUAL SNAME_POWER OR ENERGYMON_BUILD_DEFAULT STREQUAL LNAME_POWER)
  add_energymon_default_library(SOURCES ${SOURCES_POWER})
  target_link_libraries(energymon-default PRIVATE Sensors::Sensors Threads::Threads)
  target_compile_definitions(energymon-default PRIVATE ENERGYMON_IBMPOWERNV_USE_POWER)
  add_energymon_pkg_config(energymon-default "${DESCRIPTION_POWER}" "" "${PKG_CONFIG_PRIVATE_LIBS_POWER}")
  energymon_export_dependency(Threads)
  energymon_export_dependency(Sensors)
endif()
