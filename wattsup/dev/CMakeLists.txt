set(LNAME energymon-wattsup)
set(SOURCES ../energymon-wattsup.c;wattsup-driver-dev.c;${ENERGYMON_UTIL};${ENERGYMON_TIME_UTIL})
set(DESCRIPTION "EnergyMon implementation for WattsUp? Power meters")
set(SHMEM_PROVIDER ${LNAME}-shmem-provider)

# Dependencies

include_directories(..)

# Libraries

add_library(${LNAME} ${SOURCES})
target_link_libraries(${LNAME} ${CMAKE_THREAD_LIBS_INIT} ${LIBRT})
PKG_CONFIG("${LNAME}" "${DESCRIPTION}" "" "${PKG_CONFIG_PRIVATE_LIBS}")
install(TARGETS ${LNAME} DESTINATION lib)

if(DEFAULT STREQUAL "wattsup" OR DEFAULT STREQUAL LNAME)
  BUILD_DEFAULT("${SOURCES}" "${CMAKE_THREAD_LIBS_INIT};${LIBRT}" "")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "" "${PKG_CONFIG_PRIVATE_LIBS}")
  install(TARGETS energymon-default DESTINATION lib)
endif()

# Binaries

if(UNIX)
  add_executable(${SHMEM_PROVIDER} ../energymon-wattsup-shmem-provider.c)
  target_link_libraries(${SHMEM_PROVIDER} ${LNAME})
  install(TARGETS ${SHMEM_PROVIDER} DESTINATION bin)
endif()
