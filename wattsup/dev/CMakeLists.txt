set(SNAME wattsup)
set(LNAME energymon-wattsup)
set(SOURCES ../energymon-wattsup.c;wattsup-driver-dev.c;${ENERGYMON_UTIL};${ENERGYMON_TIME_UTIL})
set(DESCRIPTION "EnergyMon implementation for WattsUp? Power meters")
set(SHMEM_PROVIDER ${LNAME}-shmem-provider)

# Libraries

if(ENERGYMON_BUILD_LIB STREQUAL "ALL" OR
   ENERGYMON_BUILD_LIB STREQUAL SNAME OR
   ENERGYMON_BUILD_LIB STREQUAL LNAME)

  add_library(${LNAME} ${SOURCES})
  target_include_directories(${LNAME} PRIVATE ${PROJECT_SOURCE_DIR}/inc ${PROJECT_SOURCE_DIR}/common ..)
  target_link_libraries(${LNAME} PRIVATE Threads::Threads ${LIBRT})
  if(BUILD_SHARED_LIBS)
    set_target_properties(${LNAME} PROPERTIES VERSION ${PROJECT_VERSION}
                                              SOVERSION ${PROJECT_VERSION_MAJOR})
  endif()
  PKG_CONFIG("${LNAME}" "${DESCRIPTION}" "" "${PKG_CONFIG_PRIVATE_LIBS}")
  install(TARGETS ${LNAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})

  if(UNIX AND ENERGYMON_BUILD_SHMEM_PROVIDERS)
    add_executable(${SHMEM_PROVIDER} ../energymon-wattsup-shmem-provider.c)
    target_include_directories(${SHMEM_PROVIDER} PRIVATE ${PROJECT_SOURCE_DIR}/inc ..)
    target_link_libraries(${SHMEM_PROVIDER} PRIVATE ${LNAME})
    install(TARGETS ${SHMEM_PROVIDER} DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()

endif()

if(ENERGYMON_BUILD_DEFAULT STREQUAL SNAME OR ENERGYMON_BUILD_DEFAULT STREQUAL LNAME)
  BUILD_DEFAULT("${SOURCES}" ".." "" "Threads::Threads;${LIBRT}" "")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "" "${PKG_CONFIG_PRIVATE_LIBS}")
endif()

# Utils

CONFIGURE_ENERGYMON_GET_C(energymon-wattsup.h energymon_get_wattsup ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
BUILD_UTILS(${SNAME} ".." ${LNAME} ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
BUILD_TESTS(${SNAME} ".." ${LNAME} ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
