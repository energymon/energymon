set(SNAME wattsup-libftdi)
set(LNAME energymon-wattsup-libftdi)
set(SOURCES_LIBFTDI ../energymon-wattsup.c;wattsup-driver-libftdi.c;${ENERGYMON_UTIL};${ENERGYMON_TIME_UTIL})
set(DESCRIPTION_FTDI "EnergyMon implementation for WattsUp? Power meters using libftdi")
set(SHMEM_PROVIDER ${LNAME}-shmem-provider)

# Dependencies

set(FTDI_IMPL libftdi1)

find_package(PkgConfig)
if(${PKG_CONFIG_FOUND})
  pkg_search_module(LIBFTDI IMPORTED_TARGET libftdi1)
  if(NOT LIBFTDI_FOUND)
    set(FTDI_IMPL libftdi)
    pkg_search_module(LIBFTDI IMPORTED_TARGET libftdi)
  endif()
endif()

if(NOT LIBFTDI_FOUND)
  # fail gracefully
  message(WARNING "${LNAME}: No libftdi1 or libftdi - skipping this project")
  return()
endif()

# Libraries

if(ENERGYMON_BUILD_LIB STREQUAL "ALL" OR
   ENERGYMON_BUILD_LIB STREQUAL SNAME OR
   ENERGYMON_BUILD_LIB STREQUAL LNAME)

  add_library(${LNAME} ${SOURCES_LIBFTDI})
  target_include_directories(${LNAME} PRIVATE ${PROJECT_SOURCE_DIR}/inc ${PROJECT_SOURCE_DIR}/common ..)
  target_link_libraries(${LNAME} PRIVATE Threads::Threads ${LIBRT} PkgConfig::LIBFTDI)
  if(BUILD_SHARED_LIBS)
    set_target_properties(${LNAME} PROPERTIES VERSION ${PROJECT_VERSION}
                                              SOVERSION ${PROJECT_VERSION_MAJOR})
  endif()
  PKG_CONFIG("${LNAME}" "${DESCRIPTION_FTDI}" "${FTDI_IMPL}" "${PKG_CONFIG_PRIVATE_LIBS}")
  install(TARGETS ${LNAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})

  if(UNIX AND ENERGYMON_BUILD_SHMEM_PROVIDERS)
    add_executable(${SHMEM_PROVIDER} ../energymon-wattsup-shmem-provider.c)
    target_include_directories(${SHMEM_PROVIDER} PRIVATE ${PROJECT_SOURCE_DIR}/inc)
    target_link_libraries(${SHMEM_PROVIDER} PRIVATE ${LNAME})
    install(TARGETS ${SHMEM_PROVIDER} DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()

endif()

if(ENERGYMON_BUILD_DEFAULT STREQUAL SNAME OR ENERGYMON_BUILD_DEFAULT STREQUAL LNAME)
  BUILD_DEFAULT("${SOURCES_LIBFTDI}" ".." "" "Threads::Threads;${LIBRT};PkgConfig::LIBFTDI" "")
  PKG_CONFIG("energymon-default" "${DESCRIPTION_FTDI}" "${FTDI_IMPL}" "${PKG_CONFIG_PRIVATE_LIBS}")
endif()

# Utils

CONFIGURE_ENERGYMON_GET_C(energymon-wattsup.h energymon_get_wattsup ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
BUILD_UTILS(${SNAME} ".." ${LNAME} ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
BUILD_TESTS(${SNAME} ".." ${LNAME} ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
