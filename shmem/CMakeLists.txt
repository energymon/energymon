set(SNAME shmem)
set(LNAME energymon-shmem)
set(EXAMPLE energymon-shmem-example)
set(SOURCES ${LNAME}.c;${ENERGYMON_UTIL})
set(DESCRIPTION "EnergyMon over Shared Memory")

# Libraries

if(ENERGYMON_BUILD_LIB STREQUAL "ALL" OR
   ENERGYMON_BUILD_LIB STREQUAL SNAME OR
   ENERGYMON_BUILD_LIB STREQUAL LNAME)

  add_library(${LNAME} ${SOURCES})
  if(BUILD_SHARED_LIBS)
    set_target_properties(${LNAME} PROPERTIES VERSION ${PROJECT_VERSION}
                                              SOVERSION ${VERSION_MAJOR})
  endif()
  PKG_CONFIG("${LNAME}" "${DESCRIPTION}" "" "")
  install(TARGETS ${LNAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
  install(FILES ${LNAME}.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
endif()

if(ENERGYMON_BUILD_DEFAULT STREQUAL SNAME OR ENERGYMON_BUILD_DEFAULT STREQUAL LNAME)
  BUILD_DEFAULT("${SOURCES}" "" "")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "" "")
endif()

# Binaries

if(TARGET energymon-default AND ENERGYMON_BUILD_EXAMPLES)
  add_executable(${EXAMPLE} example/${EXAMPLE}.c)
  target_link_libraries(${EXAMPLE} energymon-default)
endif()
