set(LNAME energymon-dummy)
set(SOURCES ${LNAME}.c;../src/energymon-util.c)
set(DESCRIPTION "Dummy EnergyMon implementation")

# Libraries

add_library(${LNAME} SHARED ${SOURCES})
SET_FPIC(${LNAME})

add_library(${LNAME}-static STATIC ${SOURCES})
SET_FPIC(${LNAME}-static)

if(DEFAULT STREQUAL "dummy")
  BUILD_DEFAULT("${SOURCES}" "" "")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "")
  PKG_CONFIG("energymon-default-static" "${DESCRIPTION}" "")
  install(TARGETS energymon-default energymon-default-static DESTINATION lib)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/energymon-default.pc ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/energymon-default-static.pc DESTINATION lib/pkgconfig)
endif()

# pkg-config

PKG_CONFIG("${LNAME}" "${DESCRIPTION}" "")
PKG_CONFIG("${LNAME}-static" "${DESCRIPTION}" "")

# Install

install(TARGETS ${LNAME} ${LNAME}-static DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME}.h DESTINATION include/${PROJECT_NAME})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/${LNAME}.pc ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/${LNAME}-static.pc DESTINATION lib/pkgconfig)
