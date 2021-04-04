set(SNAME wattsup-libusb)
set(LNAME energymon-wattsup-libusb)
set(SOURCES ../energymon-wattsup.c;wattsup-driver-libusb.c;${ENERGYMON_UTIL};${ENERGYMON_TIME_UTIL})
set(DESCRIPTION "EnergyMon implementation for WattsUp? Power meters using libusb-1.0")
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

  add_energymon_library(${LNAME} ${SNAME}
                        SOURCES ${SOURCES}
                        PUBLIC_HEADER ../energymon-wattsup.h
                        PUBLIC_BUILD_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..
                        ENERGYMON_GET_HEADER energymon-wattsup.h
                        ENERGYMON_GET_FUNCTION "energymon_get_wattsup"
                        ENERGYMON_GET_C_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
  target_include_directories(${LNAME} PRIVATE ..)
  target_link_libraries(${LNAME} PRIVATE Threads::Threads ${LIBRT} PkgConfig::LIBUSB_1)
  add_energymon_pkg_config(${LNAME} "${DESCRIPTION}" "libusb-1.0" "${PKG_CONFIG_PRIVATE_LIBS}")

  set(BUILD_ATTACH_KERNEL TRUE)
  if(UNIX)
    add_executable(${SHMEM_PROVIDER} ../energymon-wattsup-shmem-provider.c)
    target_link_libraries(${SHMEM_PROVIDER} PRIVATE ${LNAME})
    install(TARGETS ${SHMEM_PROVIDER}
            EXPORT EnergyMonTargets
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()

endif()

if(ENERGYMON_BUILD_DEFAULT STREQUAL SNAME OR ENERGYMON_BUILD_DEFAULT STREQUAL LNAME)
  add_energymon_default_library(SOURCES ${SOURCES})
  target_include_directories(energymon-default PRIVATE ..)
  target_link_libraries(energymon-default PRIVATE Threads::Threads ${LIBRT} PkgConfig::LIBUSB_1)
  add_energymon_pkg_config(energymon-default "${DESCRIPTION}" "libusb-1.0" "${PKG_CONFIG_PRIVATE_LIBS}")
  set(BUILD_ATTACH_KERNEL TRUE)
endif()

# Binaries

if(BUILD_ATTACH_KERNEL)
  add_executable(energymon-wattsup-attach-kernel wattsup-attach-kernel.c)
  target_link_libraries(energymon-wattsup-attach-kernel PRIVATE PkgConfig::LIBUSB_1)
  install(TARGETS energymon-wattsup-attach-kernel
          EXPORT EnergyMonTargets
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
