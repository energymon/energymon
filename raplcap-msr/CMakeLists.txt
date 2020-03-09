set(SNAME raplcap-msr)
set(LNAME energymon-raplcap-msr)
set(SOURCES ${LNAME}.c;${ENERGYMON_UTIL})
set(DESCRIPTION "EnergyMon implementation using libraplcap-msr")

# Dependencies

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

find_package(PkgConfig)
if(${PKG_CONFIG_FOUND})
  pkg_search_module(RAPLCAP raplcap-msr>=0.3.0)
endif()
if(NOT RAPLCAP_FOUND)
  # fail gracefully
  message(WARNING "${LNAME}: Missing raplcap-msr - skipping this project")
  return()
endif()
include_directories(${RAPLCAP_INCLUDE_DIRS})

# Libraries

if(ENERGYMON_BUILD_LIB STREQUAL "ALL" OR
   ENERGYMON_BUILD_LIB STREQUAL SNAME OR
   ENERGYMON_BUILD_LIB STREQUAL LNAME)

  add_library(${LNAME} ${SOURCES})
  target_link_libraries(${LNAME} -L${RAPLCAP_LIBDIR} ${RAPLCAP_LIBRARIES})
  if(BUILD_SHARED_LIBS)
    set_target_properties(${LNAME} PROPERTIES VERSION ${PROJECT_VERSION}
                                              SOVERSION ${VERSION_MAJOR})
  endif()
  PKG_CONFIG("${LNAME}" "${DESCRIPTION}" "${RAPLCAP_LIBRARIES}" "")
  install(TARGETS ${LNAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
  install(FILES ${LNAME}.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

endif()

if(ENERGYMON_BUILD_DEFAULT STREQUAL SNAME OR ENERGYMON_BUILD_DEFAULT STREQUAL LNAME)
  BUILD_DEFAULT("${SOURCES}" "${RAPLCAP_LIBRARIES}" "")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "${RAPLCAP_LIBRARIES}" "")
endif()

# Utils

CONFIGURE_ENERGYMON_GET_C(${LNAME}.h energymon_get_raplcap_msr ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
BUILD_UTILS(${SNAME} ${LNAME} ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
BUILD_TESTS(${SNAME} ${LNAME} ${CMAKE_CURRENT_BINARY_DIR}/${LNAME}/energymon-get.c)
