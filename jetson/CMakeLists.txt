if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Linux|Android")
  return()
endif()

set(SNAME jetson)
set(LNAME energymon-jetson)
set(SOURCES ${LNAME}.c;${ENERGYMON_UTIL};${ENERGYMON_TIME_UTIL};util.c;ina3221.c;ina3221x.c)
set(DESCRIPTION "EnergyMon implementation for NVIDIA Jetson systems")

# Dependencies

find_package(Threads)
if(NOT Threads_FOUND)
  # fail gracefully
  message(WARNING "${LNAME}: Missing Threads library - skipping this project")
  return()
endif()
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
                        ENERGYMON_GET_FUNCTION "energymon_get_jetson"
                        ENERGYMON_GET_C_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
  target_link_libraries(${LNAME} PRIVATE Threads::Threads ${LIBRT})
  add_energymon_pkg_config(${LNAME} "${DESCRIPTION}" "" "${PKG_CONFIG_PRIVATE_LIBS}")
  energymon_export_dependency(Threads)

endif()

if(ENERGYMON_BUILD_DEFAULT STREQUAL SNAME OR ENERGYMON_BUILD_DEFAULT STREQUAL LNAME)
  add_energymon_default_library(SOURCES ${SOURCES})
  target_link_libraries(energymon-default PRIVATE Threads::Threads ${LIBRT})
  add_energymon_pkg_config(energymon-default "${DESCRIPTION}" "" "${PKG_CONFIG_PRIVATE_LIBS}")
  energymon_export_dependency(Threads)
endif()
