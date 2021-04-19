set(SNAME wattsup-libftdi)
set(LNAME energymon-wattsup-libftdi)
set(SOURCES ../energymon-wattsup.c;wattsup-driver-libftdi.c;${ENERGYMON_UTIL};${ENERGYMON_TIME_UTIL})
set(DESCRIPTION "EnergyMon implementation for WattsUp? Power meters using libftdi")
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

# check for ftdi_tcioflush
try_compile(HAS_FTDI_TCIOFLUSH ${CMAKE_CURRENT_BINARY_DIR}/has_ftdi_tcioflush
            SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/has_ftdi_tcioflush.c
            LINK_LIBRARIES PkgConfig::LIBFTDI)
if(HAS_FTDI_TCIOFLUSH)
  set(FTDI_IMPL "${FTDI_IMPL} >= 1.5")
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
  if(HAS_FTDI_TCIOFLUSH)
    target_compile_definitions(${LNAME} PRIVATE HAS_FTDI_TCIOFLUSH)
  endif()
  target_link_libraries(${LNAME} PRIVATE Threads::Threads ${LIBRT} PkgConfig::LIBFTDI)
  add_energymon_pkg_config(${LNAME} "${DESCRIPTION}" "${FTDI_IMPL}" "${PKG_CONFIG_PRIVATE_LIBS}")

  if(UNIX AND ENERGYMON_BUILD_SHMEM_PROVIDERS)
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
  if(HAS_FTDI_TCIOFLUSH)
    target_compile_definitions(energymon-default PRIVATE HAS_FTDI_TCIOFLUSH)
  endif()
  target_link_libraries(energymon-default PRIVATE Threads::Threads ${LIBRT} PkgConfig::LIBFTDI)
  add_energymon_pkg_config(energymon-default "${DESCRIPTION}" "${FTDI_IMPL}" "${PKG_CONFIG_PRIVATE_LIBS}")
endif()
