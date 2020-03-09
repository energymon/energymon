set(SNAME osp)
set(LNAME energymon-osp)
set(SNAME_POLLING osp-polling)
set(LNAME_POLLING energymon-osp-polling)
set(SHMEM_PROVIDER ${LNAME_POLLING}-shmem-provider)
set(SOURCES ${LNAME}.c;${ENERGYMON_UTIL};${ENERGYMON_TIME_UTIL})
set(SOURCES_POLLING ${SOURCES})
set(DESCRIPTION "EnergyMon implementation for ODROID Smart Power")
set(DESCRIPTION_POLLING "EnergyMon implementation for ODROID Smart Power with Polling")

# Dependencies

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

find_package(PkgConfig)
if(${PKG_CONFIG_FOUND})
  # Check for impl with libusb backend first
  # hidapi docs only specify it for Linux and FreeBSD, but libusb is quite portable
  set(HIDAPI_IMPL hidapi-libusb)
  pkg_search_module(HIDAPI ${HIDAPI_IMPL})
  if(NOT HIDAPI_FOUND)
    # Now look for more platform-specific implementations
    if(${CMAKE_SYSTEM_NAME} MATCHES "Linux|Android")
      # Check for impl with hidraw backend
      set(HIDAPI_IMPL hidapi-hidraw)
      pkg_search_module(HIDAPI ${HIDAPI_IMPL})
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      # Check for impl with IOHidManager backend
      set(HIDAPI_IMPL hidapi)
      pkg_search_module(HIDAPI ${HIDAPI_IMPL})
    elseif(WIN32)
      # Check for impl with dll backend (hidapi.dll)
      set(HIDAPI_IMPL hidapi)
      pkg_search_module(HIDAPI ${HIDAPI_IMPL})
    endif()
  endif()
endif()
if(NOT HIDAPI_FOUND)
  # fail gracefully
  message(WARNING "${LNAME}: Missing hidapi-libusb and ${HIDAPI_IMPL} - skipping this project")
  return()
endif()
include_directories(${HIDAPI_INCLUDE_DIRS})

# Must be set to OFF to prevent doing a try_run() during cross-compiling
set(THREADS_PTHREAD_ARG OFF)
find_package(Threads)
if(CMAKE_THREAD_LIBS_INIT)
  set(PKG_CONFIG_PRIVATE_LIBS "${CMAKE_THREAD_LIBS_INIT}")
endif()
if(LIBRT)
  set(PKG_CONFIG_PRIVATE_LIBS "${PKG_CONFIG_PRIVATE_LIBS} -lrt")
endif()

# Libraries

if(ENERGYMON_BUILD_LIB STREQUAL "ALL" OR
   ENERGYMON_BUILD_LIB STREQUAL SNAME OR
   ENERGYMON_BUILD_LIB STREQUAL LNAME)

  add_library(${LNAME} ${SOURCES})
  target_link_libraries(${LNAME} -L${HIDAPI_LIBDIR} ${HIDAPI_LIBRARIES})
  if(BUILD_SHARED_LIBS)
    set_target_properties(${LNAME} PROPERTIES VERSION ${PROJECT_VERSION}
                                              SOVERSION ${VERSION_MAJOR})
  endif()
  PKG_CONFIG("${LNAME}" "${DESCRIPTION}" "${HIDAPI_LIBRARIES}" "")
  install(TARGETS ${LNAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
  install(FILES ${LNAME}.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

endif()

if(ENERGYMON_BUILD_LIB STREQUAL "ALL" OR
   ENERGYMON_BUILD_LIB STREQUAL SNAME_POLLING OR
   ENERGYMON_BUILD_LIB STREQUAL LNAME_POLLING)

  add_library(${LNAME_POLLING} ${SOURCES_POLLING})
  set_target_properties(${LNAME_POLLING} PROPERTIES COMPILE_DEFINITIONS "ENERGYMON_OSP_USE_POLLING")
  target_link_libraries(${LNAME_POLLING} -L${HIDAPI_LIBDIR} ${HIDAPI_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${LIBRT})
  if(BUILD_SHARED_LIBS)
    set_target_properties(${LNAME_POLLING} PROPERTIES VERSION ${PROJECT_VERSION}
                                                      SOVERSION ${VERSION_MAJOR})
  endif()
  PKG_CONFIG("${LNAME_POLLING}" "${DESCRIPTION_POLLING}" "${HIDAPI_LIBRARIES}" "${PKG_CONFIG_PRIVATE_LIBS}")
  install(TARGETS ${LNAME_POLLING} DESTINATION ${CMAKE_INSTALL_LIBDIR})
  install(FILES ${LNAME_POLLING}.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

  if (ENERGYMON_BUILD_SHMEM_PROVIDERS)
    add_executable(${SHMEM_PROVIDER} energymon-osp-polling-shmem-provider.c)
    target_link_libraries(${SHMEM_PROVIDER} ${LNAME_POLLING})
    install(TARGETS ${SHMEM_PROVIDER} DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()

endif()

if(ENERGYMON_BUILD_DEFAULT STREQUAL SNAME OR ENERGYMON_BUILD_DEFAULT STREQUAL LNAME)
  BUILD_DEFAULT("${SOURCES}" "-L${HIDAPI_LIBDIR};${HIDAPI_LIBRARIES}" "")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "${HIDAPI_LIBRARIES}" "")
elseif(ENERGYMON_BUILD_DEFAULT STREQUAL SNAME_POLLING OR ENERGYMON_BUILD_DEFAULT STREQUAL LNAME_POLLING)
  BUILD_DEFAULT("${SOURCES_POLLING}" "-L${HIDAPI_LIBDIR};${HIDAPI_LIBRARIES};${CMAKE_THREAD_LIBS_INIT};${LIBRT}" "ENERGYMON_OSP_USE_POLLING")
  PKG_CONFIG("energymon-default" "${DESCRIPTION_POLLING}" "${HIDAPI_IMPL}" "${PKG_CONFIG_PRIVATE_LIBS}")
endif()

# Utils

CONFIGURE_ENERGYMON_GET_C(${LNAME}.h energymon_get_osp ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
BUILD_UTILS(${SNAME} ${LNAME} ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
BUILD_TESTS(${SNAME} ${LNAME} ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)

CONFIGURE_ENERGYMON_GET_C(${LNAME_POLLING}.h energymon_get_osp_polling ${CMAKE_CURRENT_BINARY_DIR}/${LNAME_POLLING}/energymon-get.c)
BUILD_UTILS(${SNAME_POLLING} ${LNAME_POLLING} ${CMAKE_CURRENT_BINARY_DIR}/${LNAME_POLLING}/energymon-get.c)
BUILD_TESTS(${SNAME_POLLING} ${LNAME_POLLING} ${CMAKE_CURRENT_BINARY_DIR}/${LNAME_POLLING}/energymon-get.c)
