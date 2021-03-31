set(SNAME wattsup-libusb)
set(LNAME energymon-wattsup-libusb)
set(SOURCES_LIBUSB ../energymon-wattsup.c;wattsup-driver-libusb.c;${ENERGYMON_UTIL};${ENERGYMON_TIME_UTIL})
set(DESCRIPTION_LIBUSB "EnergyMon implementation for WattsUp? Power meters using libusb-1.0")
set(SHMEM_PROVIDER ${LNAME}-shmem-provider)

# Dependencies

find_package(PkgConfig)
if(${PKG_CONFIG_FOUND})
  pkg_search_module(LIBUSB_1 IMPORTED_TARGET libusb-1.0)
endif()

if(NOT LIBUSB_1_FOUND)
  # fail gracefully
  message(WARNING "${LNAME}: No libusb-1.0 - skipping this project")
  return()
endif()

# Libraries

if(ENERGYMON_BUILD_LIB STREQUAL "ALL" OR
   ENERGYMON_BUILD_LIB STREQUAL SNAME OR
   ENERGYMON_BUILD_LIB STREQUAL LNAME)

  add_library(${LNAME} ${SOURCES_LIBUSB})
  target_include_directories(${LNAME} PRIVATE ${PROJECT_SOURCE_DIR}/inc ${PROJECT_SOURCE_DIR}/common ..)
  target_link_libraries(${LNAME} PRIVATE Threads::Threads ${LIBRT} PkgConfig::LIBUSB_1)
  if(BUILD_SHARED_LIBS)
    set_target_properties(${LNAME} PROPERTIES VERSION ${PROJECT_VERSION}
                                              SOVERSION ${PROJECT_VERSION_MAJOR})
  endif()
  PKG_CONFIG("${LNAME}" "${DESCRIPTION_LIBUSB}" "libusb-1.0" "${PKG_CONFIG_PRIVATE_LIBS}")
  install(TARGETS ${LNAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})

  set(BUILD_ATTACH_KERNEL TRUE)
  if(UNIX)
    add_executable(${SHMEM_PROVIDER} ../energymon-wattsup-shmem-provider.c)
    target_include_directories(${SHMEM_PROVIDER} PRIVATE ${PROJECT_SOURCE_DIR}/inc)
    target_link_libraries(${SHMEM_PROVIDER} PRIVATE ${LNAME})
    install(TARGETS ${SHMEM_PROVIDER} DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()

endif()

if(ENERGYMON_BUILD_DEFAULT STREQUAL SNAME OR ENERGYMON_BUILD_DEFAULT STREQUAL LNAME)
  BUILD_DEFAULT("${SOURCES_LIBUSB}" "" "" "Threads::Threads;${LIBRT};PkgConfig::LIBUSB_1" "")
  PKG_CONFIG("energymon-default" "${DESCRIPTION_LIBUSB}" "libusb-1.0" "${PKG_CONFIG_PRIVATE_LIBS}")
  set(BUILD_ATTACH_KERNEL TRUE)
endif()

# Binaries

if(BUILD_ATTACH_KERNEL)
  add_executable(energymon-wattsup-attach-kernel wattsup-attach-kernel.c)
  target_link_libraries(energymon-wattsup-attach-kernel PRIVATE PkgConfig::LIBUSB_1)
  install(TARGETS energymon-wattsup-attach-kernel DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# Utils

CONFIGURE_ENERGYMON_GET_C(energymon-wattsup.h energymon_get_wattsup ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
BUILD_UTILS(${SNAME} ".." ${LNAME} ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
BUILD_TESTS(${SNAME} ".." ${LNAME} ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
