set(SNAME odroid)
set(LNAME energymon-odroid)
set(SNAME_IOCTL odroid-ioctl)
set(LNAME_IOCTL energymon-odroid-ioctl)
set(SOURCES ${LNAME}.c;${ENERGYMON_UTIL};${ENERGYMON_TIME_UTIL})
set(SOURCES_IOCTL ${LNAME_IOCTL}.c;${ENERGYMON_UTIL};${ENERGYMON_TIME_UTIL})
set(DESCRIPTION "EnergyMon implementation for ODROID systems")
set(DESCRIPTION_IOCTL "EnergyMon implementation for ODROID systems using ioctl")

# Dependencies

find_package(Threads)
if(NOT Threads_FOUND)
  # fail gracefully
  message(WARNING "${LNAME}: Missing Threads library - skipping this project")
  message(WARNING "${LNAME_IOCTL}: Missing Threads library - skipping this project")
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
                        ENERGYMON_GET_FUNCTION "energymon_get_odroid"
                        ENERGYMON_GET_C_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
  target_link_libraries(${LNAME} PRIVATE Threads::Threads ${LIBRT})
  add_energymon_pkg_config(${LNAME} "${DESCRIPTION}" "" "${PKG_CONFIG_PRIVATE_LIBS}")

endif()

if(ENERGYMON_BUILD_LIB STREQUAL "ALL" OR
   ENERGYMON_BUILD_LIB STREQUAL SNAME_IOCTL OR
   ENERGYMON_BUILD_LIB STREQUAL LNAME_IOCTL)

  add_energymon_library(${LNAME_IOCTL} ${SNAME_IOCTL}
                        SOURCES ${SOURCES_IOCTL}
                        PUBLIC_HEADER ${LNAME_IOCTL}.h
                        PUBLIC_BUILD_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                        ENERGYMON_GET_HEADER ${LNAME_IOCTL}.h
                        ENERGYMON_GET_FUNCTION "energymon_get_odroid_ioctl"
                        ENERGYMON_GET_C_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${LNAME_IOCTL}/energymon-get.c)
  target_link_libraries(${LNAME_IOCTL} PRIVATE Threads::Threads ${LIBRT})
  add_energymon_pkg_config(${LNAME_IOCTL} "${DESCRIPTION_IOCTL}" "" "${PKG_CONFIG_PRIVATE_LIBS}")

endif()

if(ENERGYMON_BUILD_DEFAULT STREQUAL SNAME OR ENERGYMON_BUILD_DEFAULT STREQUAL LNAME)
  add_energymon_default_library(SOURCES ${SOURCES})
  target_link_libraries(energymon-default PRIVATE Threads::Threads ${LIBRT})
  add_energymon_pkg_config(energymon-default "${DESCRIPTION}" "" "${PKG_CONFIG_PRIVATE_LIBS}")
elseif(ENERGYMON_BUILD_DEFAULT STREQUAL SNAME_IOCTL OR ENERGYMON_BUILD_DEFAULT STREQUAL LNAME_IOCTL)
  add_energymon_default_library(SOURCES ${SOURCES_IOCTL})
  target_link_libraries(energymon-default PRIVATE Threads::Threads ${LIBRT})
  add_energymon_pkg_config(energymon-default "${DESCRIPTION_IOCTL}" "" "${PKG_CONFIG_PRIVATE_LIBS}")
endif()
