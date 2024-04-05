if(NOT (UNIX OR WIN32))
  return()
endif()

set(SNAME wattsup-libftdi)
set(LNAME energymon-wattsup-libftdi)
set(SOURCES ../energymon-wattsup.c;wattsup-driver-libftdi.c;${ENERGYMON_UTIL};${ENERGYMON_TIME_UTIL})
set(DESCRIPTION "EnergyMon implementation for WattsUp? Power meters using libftdi")

# Dependencies

if(NOT Threads_FOUND)
  # fail gracefully
  message(WARNING "${LNAME}: Missing Threads library - skipping this project")
  return()
endif()

set(FTDI_IMPL libftdi1)
find_package(PkgConfig)
if(${PKG_CONFIG_FOUND})
  pkg_search_module(LIBFTDI IMPORTED_TARGET ${FTDI_IMPL})
  if(NOT LIBFTDI_FOUND)
    set(FTDI_IMPL libftdi)
    pkg_search_module(LIBFTDI IMPORTED_TARGET ${FTDI_IMPL})
    if(LIBFTDI_FOUND)
      message(WARNING "${LNAME}: Support for the old libftdi is deprecated and will be removed in a future release (use libftdi1 instead).")
    endif()
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
  set(PKG_CONFIG_EXPORT_SPEC "${FTDI_IMPL}>=1.5")
  set(PKG_CONFIG_REQUIRES_PRIVATE "${FTDI_IMPL} >= 1.5")
else()
  set(PKG_CONFIG_EXPORT_SPEC ${FTDI_IMPL})
  set(PKG_CONFIG_REQUIRES_PRIVATE ${FTDI_IMPL})
endif()

# Libraries

if(ENERGYMON_BUILD_LIB STREQUAL "ALL" OR
   ENERGYMON_BUILD_LIB STREQUAL SNAME OR
   ENERGYMON_BUILD_LIB STREQUAL LNAME)

  add_energymon_library(${LNAME} ${SNAME}
                        BUILD_SHMEM_PROVIDER
                        SOURCES ${SOURCES}
                        PUBLIC_HEADER ../energymon-wattsup.h
                        PUBLIC_BUILD_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..
                        ENERGYMON_GET_HEADER energymon-wattsup.h
                        ENERGYMON_GET_FUNCTION "energymon_get_wattsup"
                        ENERGYMON_GET_C_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
  target_include_directories(${LNAME} PRIVATE ..)
  target_compile_definitions(${LNAME} PRIVATE HAS_FTDI_TCIOFLUSH=$<BOOL:${HAS_FTDI_TCIOFLUSH}>)
  target_link_libraries(${LNAME} PRIVATE Threads::Threads ${LIBRT} PkgConfig::LIBFTDI)
  add_energymon_pkg_config(${LNAME} "${DESCRIPTION}" "${PKG_CONFIG_REQUIRES_PRIVATE}" "${PKG_CONFIG_PRIVATE_LIBS}")
  energymon_export_dependency(Threads)
  energymon_export_pkg_dependency(LIBFTDI ${PKG_CONFIG_EXPORT_SPEC} IMPORTED_TARGET)

endif()

if(ENERGYMON_BUILD_DEFAULT STREQUAL SNAME OR ENERGYMON_BUILD_DEFAULT STREQUAL LNAME)
  add_energymon_default_library(SOURCES ${SOURCES})
  target_include_directories(energymon-default PRIVATE ..)
  target_compile_definitions(energymon-default PRIVATE HAS_FTDI_TCIOFLUSH=$<BOOL:${HAS_FTDI_TCIOFLUSH}>)
  target_link_libraries(energymon-default PRIVATE Threads::Threads ${LIBRT} PkgConfig::LIBFTDI)
  add_energymon_pkg_config(energymon-default "${DESCRIPTION}" "${PKG_CONFIG_REQUIRES_PRIVATE}" "${PKG_CONFIG_PRIVATE_LIBS}")
  energymon_export_dependency(Threads)
  energymon_export_pkg_dependency(LIBFTDI ${PKG_CONFIG_EXPORT_SPEC} IMPORTED_TARGET)
endif()
