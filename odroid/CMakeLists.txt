set(LNAME energymon-odroid)
set(SOURCES ${LNAME}.c;../src/energymon-util.c)
set(DESCRIPTION "EnergyMon implementation for ODROID systems")

# Libraries

add_library(${LNAME} SHARED ${SOURCES})
target_link_libraries(${LNAME} pthread)

add_library(${LNAME}-static STATIC ${SOURCES})
target_link_libraries(${LNAME}-static pthread)

if(DEFAULT STREQUAL "odroid")
  BUILD_DEFAULT("${SOURCES}" "pthread" "")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "")
  PKG_CONFIG("energymon-default-static" "${DESCRIPTION}" "")
  install(TARGETS energymon-default energymon-default-static DESTINATION lib)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/energymon-default.pc ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/energymon-default-static.pc DESTINATION lib/pkgconfig)
endif()

# pkg-config

PKG_CONFIG("${LNAME}" "${DESCRIPTION}" "-lpthread")
PKG_CONFIG("${LNAME}-static" "${DESCRIPTION}" "-lpthread")

# Install

install(TARGETS ${LNAME} ${LNAME}-static DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME}.h DESTINATION include/${PROJECT_NAME})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/${LNAME}.pc ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/${LNAME}-static.pc DESTINATION lib/pkgconfig)
