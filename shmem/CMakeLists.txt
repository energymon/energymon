set(SNAME shmem)
set(LNAME energymon-shmem)
set(EXAMPLE energymon-shmem-example)
set(SOURCES ${LNAME}.c;${ENERGYMON_UTIL})
set(DESCRIPTION "EnergyMon over Shared Memory")

# Libraries

if(ENERGYMON_BUILD_LIB STREQUAL "ALL" OR
   ENERGYMON_BUILD_LIB STREQUAL SNAME OR
   ENERGYMON_BUILD_LIB STREQUAL LNAME)

  add_energymon_library(${LNAME} ${SNAME}
                        SOURCES ${SOURCES}
                        PUBLIC_HEADER ${LNAME}.h
                        PUBLIC_BUILD_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                        ENERGYMON_GET_HEADER ${LNAME}.h
                        ENERGYMON_GET_FUNCTION "energymon_get_shmem"
                        ENERGYMON_GET_C_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
  add_energymon_pkg_config(${LNAME} "${DESCRIPTION}" "" "")

endif()

if(ENERGYMON_BUILD_DEFAULT STREQUAL SNAME OR ENERGYMON_BUILD_DEFAULT STREQUAL LNAME)
  add_energymon_default_library(SOURCES ${SOURCES})
  add_energymon_pkg_config(energymon-default "${DESCRIPTION}" "" "")
endif()

# Shared Memory Providers

function(configure_energymon_shmem_provider_man SHORT_NAME UTIL_PREFIX)
  string(REPLACE "-" "\\-" MAN_BINARY_PREFIX "${UTIL_PREFIX}")
  string(TOUPPER "${MAN_BINARY_PREFIX}" MAN_BINARY_PREFIX_UPPER)
  string(REPLACE "-" "\\-" MAN_IMPL "${SHORT_NAME}")
  configure_file(
    ${PROJECT_SOURCE_DIR}/shmem/man/energymon-shmem-provider.1.in
    ${CMAKE_CURRENT_BINARY_DIR}/man/man1/${UTIL_PREFIX}-shmem-provider.1
    @ONLY
  )
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/man/man1/${UTIL_PREFIX}-shmem-provider.1
          DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
endfunction()

function(add_energymon_shmem_provider SHORT_NAME TARGET_LIB ENERGYMON_GET_C)
  if(NOT ENERGYMON_BUILD_SHMEM_PROVIDERS OR NOT UNIX)
    return()
  endif()

  set(UTIL_PREFIX "energymon-${SHORT_NAME}")
  add_executable(${UTIL_PREFIX}-shmem-provider ${PROJECT_SOURCE_DIR}/shmem/energymon-shmem-provider.c
                                               ${ENERGYMON_GET_C})
  target_include_directories(${UTIL_PREFIX}-shmem-provider PRIVATE ${PROJECT_SOURCE_DIR}/common)
  target_compile_definitions(${UTIL_PREFIX}-shmem-provider PRIVATE ENERGYMON_UTIL_PREFIX=\"${UTIL_PREFIX}\")
  target_link_libraries(${UTIL_PREFIX}-shmem-provider PRIVATE ${TARGET_LIB})
  install(TARGETS ${UTIL_PREFIX}-shmem-provider
          EXPORT EnergyMonTargets
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  configure_energymon_shmem_provider_man(${SHORT_NAME} ${UTIL_PREFIX})
endfunction()

# Binaries

if(TARGET energymon-default AND ENERGYMON_BUILD_EXAMPLES)
  add_executable(${EXAMPLE} example/${EXAMPLE}.c)
  target_link_libraries(${EXAMPLE} PRIVATE energymon-default)
endif()
