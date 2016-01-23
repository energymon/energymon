set(LNAME energymon-odroid-ioctl)
set(SOURCES ${LNAME}.c;../src/energymon-util.c;../src/energymon-time-util.c)
set(DESCRIPTION "EnergyMon implementation for ODROID systems using ioctl")

# Libraries

add_library(${LNAME} SHARED ${SOURCES})
target_link_libraries(${LNAME} pthread rt)

add_library(${LNAME}-static STATIC ${SOURCES})
target_link_libraries(${LNAME}-static pthread rt)

if(DEFAULT STREQUAL "odroid-ioctl")
  BUILD_DEFAULT("${SOURCES}" "pthread;rt" "")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "-lrt")
  PKG_CONFIG("energymon-default-static" "${DESCRIPTION}" "-lrt")
  install(TARGETS energymon-default energymon-default-static DESTINATION lib)
  install(FILES ${CMAKE_BINARY_DIR}/pkgconfig/energymon-default.pc ${CMAKE_BINARY_DIR}/pkgconfig/energymon-default-static.pc DESTINATION lib/pkgconfig)
endif()

# pkg-config

PKG_CONFIG("${LNAME}" "${DESCRIPTION}" "-lpthread -lrt")
PKG_CONFIG("${LNAME}-static" "${DESCRIPTION}" "-lpthread -lrt")

# Install

install(TARGETS ${LNAME} ${LNAME}-static DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME}.h DESTINATION include/${PROJECT_NAME})
install(FILES ${CMAKE_BINARY_DIR}/pkgconfig/${LNAME}.pc ${CMAKE_BINARY_DIR}/pkgconfig/${LNAME}-static.pc DESTINATION lib/pkgconfig)
