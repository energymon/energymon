set(SNAME osp)
set(LNAME energymon-osp)
set(SNAME_POLLING osp-polling)
set(LNAME_POLLING energymon-osp-polling)
set(SHMEM_PROVIDER ${LNAME_POLLING}-shmem-provider)
set(SOURCES ${LNAME}.c;${ENERGYMON_UTIL};${ENERGYMON_TIME_UTIL})
set(SOURCES_POLLING ${SOURCES})
set(DESCRIPTION "EnergyMon implementation for ODROID Smart Power")
set(DESCRIPTION_POLLING "EnergyMon implementation for ODROID Smart Power with Polling")

# Shared Dependencies

find_package(PkgConfig)
if(${PKG_CONFIG_FOUND})
  # Check for impl with libusb backend first
  # hidapi docs only specify it for Linux and FreeBSD, but libusb is quite portable
  set(HIDAPI_IMPL hidapi-libusb)
  pkg_search_module(HIDAPI IMPORTED_TARGET ${HIDAPI_IMPL})
  if(NOT HIDAPI_FOUND)
    # Now look for more platform-specific implementations
    if(${CMAKE_SYSTEM_NAME} MATCHES "Linux|Android")
      # Check for impl with hidraw backend
      set(HIDAPI_IMPL hidapi-hidraw)
      pkg_search_module(HIDAPI IMPORTED_TARGET ${HIDAPI_IMPL})
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      # Check for impl with IOHidManager backend
      set(HIDAPI_IMPL hidapi)
      pkg_search_module(HIDAPI IMPORTED_TARGET ${HIDAPI_IMPL})
    elseif(WIN32)
      # Check for impl with dll backend (hidapi.dll)
      set(HIDAPI_IMPL hidapi)
      pkg_search_module(HIDAPI IMPORTED_TARGET ${HIDAPI_IMPL})
    endif()
  endif()
endif()
if(NOT HIDAPI_FOUND)
  # fail gracefully
  message(WARNING "${LNAME}: Missing hidapi-libusb and ${HIDAPI_IMPL} - skipping this project")
  message(WARNING "${LNAME_POLLING}: Missing hidapi-libusb and ${HIDAPI_IMPL} - skipping this project")
  return()
endif()

if(LIBRT)
  # both implementations have timing/sleep functionality used during device comm retries
  list(APPEND PKG_CONFIG_PRIVATE_LIBS "-lrt")
  list(APPEND PKG_CONFIG_PRIVATE_LIBS_POLLING "-lrt")
endif()


# Energy Library

if(ENERGYMON_BUILD_LIB STREQUAL "ALL" OR
   ENERGYMON_BUILD_LIB STREQUAL SNAME OR
   ENERGYMON_BUILD_LIB STREQUAL LNAME)

  add_energymon_library(${LNAME} ${SNAME}
                        SOURCES ${SOURCES}
                        PUBLIC_HEADER ${LNAME}.h
                        PUBLIC_BUILD_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                        ENERGYMON_GET_HEADER ${LNAME}.h
                        ENERGYMON_GET_FUNCTION "energymon_get_osp"
                        ENERGYMON_GET_C_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
  target_link_libraries(${LNAME} PRIVATE PkgConfig::HIDAPI ${LIBRT})
  add_energymon_pkg_config(${LNAME} "${DESCRIPTION}" "${HIDAPI_IMPL}" "${PKG_CONFIG_PRIVATE_LIBS}")
  energymon_export_pkg_dependency(HIDAPI ${HIDAPI_IMPL} IMPORTED_TARGET)

endif()

if(ENERGYMON_BUILD_DEFAULT STREQUAL SNAME OR ENERGYMON_BUILD_DEFAULT STREQUAL LNAME)
  add_energymon_default_library(SOURCES ${SOURCES})
  target_link_libraries(energymon-default PRIVATE PkgConfig::HIDAPI ${LIBRT})
  add_energymon_pkg_config(energymon-default "${DESCRIPTION}" "${HIDAPI_IMPL}" "${PKG_CONFIG_PRIVATE_LIBS}")
  energymon_export_pkg_dependency(HIDAPI ${HIDAPI_IMPL} IMPORTED_TARGET)
endif()


# Power Polling Library

find_package(Threads)
if(NOT Threads_FOUND)
  # fail gracefully
  message(WARNING "${LNAME_POLLING}: Missing Threads library - skipping this project")
  return()
endif()
if(CMAKE_THREAD_LIBS_INIT)
  list(APPEND PKG_CONFIG_PRIVATE_LIBS_POLLING "${CMAKE_THREAD_LIBS_INIT}")
endif()

if(ENERGYMON_BUILD_LIB STREQUAL "ALL" OR
   ENERGYMON_BUILD_LIB STREQUAL SNAME_POLLING OR
   ENERGYMON_BUILD_LIB STREQUAL LNAME_POLLING)

  add_energymon_library(${LNAME_POLLING} ${SNAME_POLLING}
                        SOURCES ${SOURCES_POLLING}
                        PUBLIC_HEADER ${LNAME_POLLING}.h
                        PUBLIC_BUILD_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                        ENERGYMON_GET_HEADER ${LNAME_POLLING}.h
                        ENERGYMON_GET_FUNCTION "energymon_get_osp_polling"
                        ENERGYMON_GET_C_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${LNAME_POLLING}/energymon-get.c)
  target_compile_definitions(${LNAME_POLLING} PRIVATE ENERGYMON_OSP_USE_POLLING)
  target_link_libraries(${LNAME_POLLING} PRIVATE PkgConfig::HIDAPI Threads::Threads ${LIBRT})
  add_energymon_pkg_config(${LNAME_POLLING} "${DESCRIPTION_POLLING}" "${HIDAPI_IMPL}" "${PKG_CONFIG_PRIVATE_LIBS_POLLING}")
  energymon_export_pkg_dependency(HIDAPI ${HIDAPI_IMPL} IMPORTED_TARGET)
  energymon_export_dependency(Threads)

  if (ENERGYMON_BUILD_SHMEM_PROVIDERS)
    add_executable(${SHMEM_PROVIDER} energymon-osp-polling-shmem-provider.c)
    target_include_directories(${SHMEM_PROVIDER} PRIVATE ${PROJECT_SOURCE_DIR}/inc)
    target_link_libraries(${SHMEM_PROVIDER} PRIVATE ${LNAME_POLLING})
    install(TARGETS ${SHMEM_PROVIDER}
            EXPORT EnergyMonTargets
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()

endif()

if(ENERGYMON_BUILD_DEFAULT STREQUAL SNAME_POLLING OR ENERGYMON_BUILD_DEFAULT STREQUAL LNAME_POLLING)
  add_energymon_default_library(SOURCES ${SOURCES_POLLING})
  target_compile_definitions(energymon-default PRIVATE ENERGYMON_OSP_USE_POLLING)
  target_link_libraries(energymon-default PRIVATE PkgConfig::HIDAPI Threads::Threads ${LIBRT})
  add_energymon_pkg_config(energymon-default "${DESCRIPTION_POLLING}" "${HIDAPI_IMPL}" "${PKG_CONFIG_PRIVATE_LIBS_POLLING}")
  energymon_export_pkg_dependency(HIDAPI ${HIDAPI_IMPL} IMPORTED_TARGET)
  energymon_export_dependency(Threads)
endif()
