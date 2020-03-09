set(SNAME msr)
set(LNAME energymon-msr)
set(SOURCES ${LNAME}.c;${ENERGYMON_UTIL})
set(DESCRIPTION "EnergyMon implementation for Intel Model Specific Register")

# Dependencies

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

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

# Utils

CONFIGURE_ENERGYMON_GET_C(${LNAME}.h energymon_get_msr ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
BUILD_UTILS(${SNAME} ${LNAME} ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
BUILD_TESTS(${LNAME} ${LNAME} ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
