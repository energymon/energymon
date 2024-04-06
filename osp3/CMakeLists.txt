if(NOT UNIX)
  return()
endif()

set(SNAME osp3)
set(LNAME energymon-osp3)
set(SOURCES ${LNAME}.c;${ENERGYMON_UTIL})
set(DESCRIPTION "EnergyMon implementation for ODROID Smart Power 3")


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

find_package(OSP3)
if(NOT OSP3_FOUND)
  # fail gracefully
  message(WARNING "${LNAME}: Missing libosp3 - skipping this project")
  return()
endif()


# Libraries

if(ENERGYMON_BUILD_LIB STREQUAL "ALL" OR
   ENERGYMON_BUILD_LIB STREQUAL SNAME OR
   ENERGYMON_BUILD_LIB STREQUAL LNAME)

  add_energymon_library(${LNAME} ${SNAME}
                        BUILD_SHMEM_PROVIDER
                        SOURCES ${SOURCES}
                        PUBLIC_HEADER ${LNAME}.h
                        PUBLIC_BUILD_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                        ENERGYMON_GET_HEADER ${LNAME}.h
                        ENERGYMON_GET_FUNCTION "energymon_get_osp3"
                        ENERGYMON_GET_C_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
  target_link_libraries(${LNAME} PRIVATE OSP3::osp3 Threads::Threads)
  add_energymon_pkg_config(${LNAME} "${DESCRIPTION}" "osp3" "${PKG_CONFIG_PRIVATE_LIBS}")
  energymon_export_dependency(OSP3)

endif()

if(ENERGYMON_BUILD_DEFAULT STREQUAL SNAME OR ENERGYMON_BUILD_DEFAULT STREQUAL LNAME)
  add_energymon_default_library(SOURCES ${SOURCES})
  target_link_libraries(energymon-default PRIVATE OSP3::osp3 Threads::Threads)
  add_energymon_pkg_config(energymon-default "${DESCRIPTION}" "osp3" "${PKG_CONFIG_PRIVATE_LIBS}")
  energymon_export_dependency(OSP3)
endif()
