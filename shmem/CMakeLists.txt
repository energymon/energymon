set(SNAME shmem)
set(LNAME energymon-shmem)
set(EXAMPLE energymon-shmem-example)
set(SOURCES ${LNAME}.c;${ENERGYMON_UTIL})
set(DESCRIPTION "EnergyMon over Shared Memory")

# Libraries

if(ENERGYMON_BUILD_LIB STREQUAL "ALL" OR
   ENERGYMON_BUILD_LIB STREQUAL SNAME OR
   ENERGYMON_BUILD_LIB STREQUAL LNAME)

  add_energymon_library(${LNAME} ${SNAME}
                        SOURCES ${SOURCES}
                        PUBLIC_HEADER ${LNAME}.h
                        PUBLIC_BUILD_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                        ENERGYMON_GET_HEADER ${LNAME}.h
                        ENERGYMON_GET_FUNCTION "energymon_get_shmem"
                        ENERGYMON_GET_C_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
  add_energymon_pkg_config(${LNAME} "${DESCRIPTION}" "" "")

endif()

if(ENERGYMON_BUILD_DEFAULT STREQUAL SNAME OR ENERGYMON_BUILD_DEFAULT STREQUAL LNAME)
  add_energymon_default_library(SOURCES ${SOURCES})
  add_energymon_pkg_config(energymon-default "${DESCRIPTION}" "" "")
endif()

# Binaries

if(TARGET energymon-default AND ENERGYMON_BUILD_EXAMPLES)
  add_executable(${EXAMPLE} example/${EXAMPLE}.c)
  target_link_libraries(${EXAMPLE} PRIVATE energymon-default)
endif()
