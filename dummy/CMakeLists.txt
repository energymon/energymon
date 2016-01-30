set(LNAME energymon-dummy)
set(SOURCES ${LNAME}.c;../src/energymon-util.c)
set(DESCRIPTION "Dummy EnergyMon implementation")

# Libraries

add_library(${LNAME} ${SOURCES})

if(DEFAULT STREQUAL "dummy" OR DEFAULT STREQUAL LNAME)
  BUILD_DEFAULT("${SOURCES}" "" "")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "" "")
  install(TARGETS energymon-default DESTINATION lib)
endif()

# pkg-config

PKG_CONFIG("${LNAME}" "${DESCRIPTION}" "" "")

# Install

install(TARGETS ${LNAME} DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME}.h DESTINATION include/${PROJECT_NAME})
