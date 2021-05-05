set(SNAME ibmpowernv-power)
set(LNAME energymon-ibmpowernv-power)
set(SOURCES ${LNAME}.c;${ENERGYMON_UTIL};${ENERGYMON_TIME_UTIL})
set(DESCRIPTION "EnergyMon implementation for IBM PowerNV systems")

# Dependencies

# libsensors 3.0.1 is first version to support hwmon class directory
# Unfortunately, libsensors doesn't have its own pkg-config file, so we can't
# enforce a min dependency version in our own generated pkg-config files.
find_package(Sensors 3.0.1)
if(NOT Sensors_FOUND)
  # fail gracefully
  message(WARNING "${LNAME}: Missing libsensors - skipping this project")
  return()
endif()
list(APPEND PKG_CONFIG_PRIVATE_LIBS "-lsensors")

find_package(Threads)
if(CMAKE_THREAD_LIBS_INIT)
  list(APPEND PKG_CONFIG_PRIVATE_LIBS "${CMAKE_THREAD_LIBS_INIT}")
endif()
if(LIBRT)
  list(APPEND PKG_CONFIG_PRIVATE_LIBS "-lrt")
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
                        ENERGYMON_GET_FUNCTION "energymon_get_ibmpowernv_power"
                        ENERGYMON_GET_C_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
  target_link_libraries(${LNAME} PRIVATE Sensors::Sensors Threads::Threads ${LIBRT})
  add_energymon_pkg_config(${LNAME} "${DESCRIPTION}" "" "${PKG_CONFIG_PRIVATE_LIBS}")
  energymon_export_dependency(Threads)
  energymon_export_dependency(Sensors)

endif()

if(ENERGYMON_BUILD_DEFAULT STREQUAL SNAME OR ENERGYMON_BUILD_DEFAULT STREQUAL LNAME)
  add_energymon_default_library(SOURCES ${SOURCES})
  target_link_libraries(energymon-default PRIVATE Sensors::Sensors Threads::Threads ${LIBRT})
  add_energymon_pkg_config(energymon-default "${DESCRIPTION}" "" "${PKG_CONFIG_PRIVATE_LIBS}")
  energymon_export_dependency(Threads)
  energymon_export_dependency(Sensors)
endif()
