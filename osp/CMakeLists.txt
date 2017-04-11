set(LNAME energymon-osp)
set(LNAME_POLLING energymon-osp-polling)
set(SHMEM_PROVIDER ${LNAME_POLLING}-shmem-provider)
set(SOURCES ${LNAME}.c;${ENERGYMON_UTIL};${ENERGYMON_TIME_UTIL})
set(SOURCES_POLLING ${SOURCES})
set(DESCRIPTION "EnergyMon implementation for ODROID Smart Power")
set(DESCRIPTION_POLLING "EnergyMon implementation for ODROID Smart Power with Polling")

# Dependencies

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

add_library(${LNAME} ${SOURCES})
target_link_libraries(${LNAME} -L${HIDAPI_LIBDIR} ${HIDAPI_LIBRARIES})

add_library(${LNAME_POLLING} ${SOURCES_POLLING})
target_link_libraries(${LNAME_POLLING} -L${HIDAPI_LIBDIR} ${HIDAPI_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${LIBRT})

if(DEFAULT STREQUAL "osp" OR DEFAULT STREQUAL LNAME)
  BUILD_DEFAULT("${SOURCES}" "-L${HIDAPI_LIBDIR};${HIDAPI_LIBRARIES}" "")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "${HIDAPI_LIBRARIES}" "")
  install(TARGETS energymon-default DESTINATION lib)
elseif(DEFAULT STREQUAL "osp-polling" OR DEFAULT STREQUAL LNAME_POLLING)
  BUILD_DEFAULT("${SOURCES_POLLING}" "-L${HIDAPI_LIBDIR};${HIDAPI_LIBRARIES};${CMAKE_THREAD_LIBS_INIT};${LIBRT}" "ENERGYMON_OSP_USE_POLLING")
  PKG_CONFIG("energymon-default" "${DESCRIPTION_POLLING}" "${HIDAPI_IMPL}" "${PKG_CONFIG_PRIVATE_LIBS}")
  install(TARGETS energymon-default DESTINATION lib)
endif()

set_target_properties(${LNAME_POLLING} PROPERTIES COMPILE_DEFINITIONS "ENERGYMON_OSP_USE_POLLING")

# pkg-config

PKG_CONFIG("${LNAME}" "${DESCRIPTION}" "${HIDAPI_LIBRARIES}" "")
PKG_CONFIG("${LNAME_POLLING}" "${DESCRIPTION_POLLING}" "${HIDAPI_LIBRARIES}" "${PKG_CONFIG_PRIVATE_LIBS}")

# Binaries

add_executable(${SHMEM_PROVIDER} energymon-osp-polling-shmem-provider.c)
target_link_libraries(${SHMEM_PROVIDER} ${LNAME_POLLING})

# Install

install(TARGETS ${LNAME} ${LNAME_POLLING} DESTINATION lib)
install(TARGETS ${SHMEM_PROVIDER} DESTINATION bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME}.h ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME_POLLING}.h
        DESTINATION include/${PROJECT_NAME})
