set(SNAME wattsup-libusb)
set(LNAME energymon-wattsup-libusb)
set(SOURCES_LIBUSB ../energymon-wattsup.c;wattsup-driver-libusb.c;${ENERGYMON_UTIL};${ENERGYMON_TIME_UTIL})
set(DESCRIPTION_LIBUSB "EnergyMon implementation for WattsUp? Power meters using libusb-1.0")
set(SHMEM_PROVIDER ${LNAME}-shmem-provider)

# Dependencies

find_package(PkgConfig)
if(${PKG_CONFIG_FOUND})
  pkg_search_module(LIBUSB_1 libusb-1.0)
endif()

if(NOT LIBUSB_1_FOUND)
  # fail gracefully
  message(WARNING "${LNAME}: No libusb-1.0 - skipping this project")
  return()
endif()

include_directories(.. ${LIBUSB_1_INCLUDE_DIRS})

# Libraries

if(ENERGYMON_BUILD_LIB STREQUAL "ALL" OR
   ENERGYMON_BUILD_LIB STREQUAL SNAME OR
   ENERGYMON_BUILD_LIB STREQUAL LNAME)

  add_library(${LNAME} ${SOURCES_LIBUSB})
  target_link_libraries(${LNAME} ${CMAKE_THREAD_LIBS_INIT} ${LIBRT} ${LIBUSB_1_LIBRARIES})
  if(BUILD_SHARED_LIBS)
    set_target_properties(${LNAME} PROPERTIES VERSION ${PROJECT_VERSION}
                                              SOVERSION ${VERSION_MAJOR})
  endif()
  PKG_CONFIG("${LNAME}" "${DESCRIPTION_LIBUSB}" "libusb-1.0" "${PKG_CONFIG_PRIVATE_LIBS}")
  install(TARGETS ${LNAME} DESTINATION lib)

  set(BUILD_ATTACH_KERNEL TRUE)
  if(UNIX)
    add_executable(${SHMEM_PROVIDER} ../energymon-wattsup-shmem-provider.c)
    target_link_libraries(${SHMEM_PROVIDER} ${LNAME})
    install(TARGETS ${SHMEM_PROVIDER} DESTINATION bin)
  endif()

endif()

if(ENERGYMON_BUILD_DEFAULT STREQUAL SNAME OR ENERGYMON_BUILD_DEFAULT STREQUAL LNAME)
  BUILD_DEFAULT("${SOURCES_LIBUSB}" "${CMAKE_THREAD_LIBS_INIT};${LIBRT};${LIBUSB_1_LIBRARIES}" "")
  PKG_CONFIG("energymon-default" "${DESCRIPTION_LIBUSB}" "libusb-1.0" "${PKG_CONFIG_PRIVATE_LIBS}")
  set(BUILD_ATTACH_KERNEL TRUE)
endif()

# Binaries

if(BUILD_ATTACH_KERNEL)
  add_executable(energymon-wattsup-attach-kernel wattsup-attach-kernel.c)
  target_link_libraries(energymon-wattsup-attach-kernel ${LIBUSB_1_LIBRARIES})
  install(TARGETS energymon-wattsup-attach-kernel DESTINATION bin)
endif()
