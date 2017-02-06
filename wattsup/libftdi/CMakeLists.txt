set(LNAME energymon-wattsup-libftdi)
set(SOURCES_LIBFTDI ../energymon-wattsup.c;wattsup-driver-libftdi.c;${ENERGYMON_UTIL};${ENERGYMON_TIME_UTIL})
set(DESCRIPTION_FTDI "EnergyMon implementation for WattsUp? Power meters using libftdi")
set(SHMEM_PROVIDER ${LNAME}-shmem-provider)

# Dependencies

set(FTDI_IMPL libftdi1)

find_package(PkgConfig)
if(${PKG_CONFIG_FOUND})
  pkg_search_module(LIBFTDI libftdi1)
  if(NOT LIBFTDI_FOUND)
    set(FTDI_IMPL libftdi)
    pkg_search_module(LIBFTDI libftdi)
  endif()
endif()

if(NOT LIBFTDI_FOUND)
  # fail gracefully
  message(WARNING "${LNAME}: No libftdi1 or libftdi - skipping this project")
  return()
endif()

include_directories(.. ${LIBFTDI_INCLUDE_DIRS})

# Libraries

add_library(${LNAME} ${SOURCES_LIBFTDI})
target_link_libraries(${LNAME} ${CMAKE_THREAD_LIBS_INIT} ${LIBRT} ${LIBFTDI_LIBRARIES})
PKG_CONFIG("${LNAME}" "${DESCRIPTION_LIBFTDI}" "${FTDI_IMPL}" "${PKG_CONFIG_PRIVATE_LIBS}")
install(TARGETS ${LNAME} DESTINATION lib)

if(DEFAULT STREQUAL "wattsup-libftdi" OR DEFAULT STREQUAL LNAME)
  BUILD_DEFAULT("${SOURCES_LIBFTDI}" "${CMAKE_THREAD_LIBS_INIT};${LIBRT};${LIBFTDI_LIBRARIES}" "")
  PKG_CONFIG("energymon-default" "${DESCRIPTION_LIBFTDI}" "${FTDI_IMPL}" "${PKG_CONFIG_PRIVATE_LIBS}")
  install(TARGETS energymon-default DESTINATION lib)
endif()

# Binaries

if(UNIX)
  add_executable(${SHMEM_PROVIDER} ../energymon-wattsup-shmem-provider.c)
  target_link_libraries(${SHMEM_PROVIDER} ${LNAME})
  install(TARGETS ${SHMEM_PROVIDER} DESTINATION bin)
endif()
