if(NOT UNIX)
  return()
endif()

set(SNAME wattsup)
set(LNAME energymon-wattsup)
set(SOURCES ../energymon-wattsup.c;wattsup-driver-dev.c;${ENERGYMON_UTIL};${ENERGYMON_TIME_UTIL})
set(DESCRIPTION "EnergyMon implementation for WattsUp? Power meters")

# Dependencies

if(NOT Threads_FOUND)
  # fail gracefully
  message(WARNING "${LNAME}: Missing Threads library - skipping this project")
  return()
endif()

# Libraries

if(ENERGYMON_BUILD_LIB STREQUAL "ALL" OR
   ENERGYMON_BUILD_LIB STREQUAL SNAME OR
   ENERGYMON_BUILD_LIB STREQUAL LNAME)

  add_energymon_library(${LNAME} ${SNAME}
                        BUILD_SHMEM_PROVIDER
                        SOURCES ${SOURCES}
                        PUBLIC_HEADER ../energymon-wattsup.h
                        PUBLIC_BUILD_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..
                        ENERGYMON_GET_HEADER energymon-wattsup.h
                        ENERGYMON_GET_FUNCTION "energymon_get_wattsup"
                        ENERGYMON_GET_C_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
  target_include_directories(${LNAME} PRIVATE ..)
  target_link_libraries(${LNAME} PRIVATE Threads::Threads ${LIBRT})
  add_energymon_pkg_config(${LNAME} "${DESCRIPTION}" "" "${PKG_CONFIG_PRIVATE_LIBS}")
  energymon_export_dependency(Threads)

endif()

if(ENERGYMON_BUILD_DEFAULT STREQUAL SNAME OR ENERGYMON_BUILD_DEFAULT STREQUAL LNAME)
  add_energymon_default_library(SOURCES ${SOURCES})
  target_include_directories(energymon-default PRIVATE ..)
  target_link_libraries(energymon-default PRIVATE Threads::Threads ${LIBRT})
  add_energymon_pkg_config(energymon-default "${DESCRIPTION}" "" "${PKG_CONFIG_PRIVATE_LIBS}")
  energymon_export_dependency(Threads)
endif()
