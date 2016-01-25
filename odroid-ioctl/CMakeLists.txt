set(LNAME energymon-odroid-ioctl)
set(SOURCES ${LNAME}.c;../src/energymon-util.c;../src/energymon-time-util.c)
set(DESCRIPTION "EnergyMon implementation for ODROID systems using ioctl")

# Libraries

add_library(${LNAME} ${SOURCES})
target_link_libraries(${LNAME} pthread rt)

if(DEFAULT STREQUAL "odroid-ioctl")
  BUILD_DEFAULT("${SOURCES}" "pthread;rt" "")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "" "-lpthread -lrt")
  install(TARGETS energymon-default DESTINATION lib)
endif()

# pkg-config

PKG_CONFIG("${LNAME}" "${DESCRIPTION}" "" "-lpthread -lrt")

# Install

install(TARGETS ${LNAME} DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME}.h DESTINATION include/${PROJECT_NAME})
