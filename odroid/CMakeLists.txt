set(LNAME energymon-odroid)
set(LNAME_IOCTL energymon-odroid-ioctl)
set(SOURCES ${LNAME}.c;../src/energymon-util.c;../src/energymon-time-util.c)
set(SOURCES_IOCTL ${LNAME_IOCTL}.c;../src/energymon-util.c;../src/energymon-time-util.c)
set(DESCRIPTION "EnergyMon implementation for ODROID systems")
set(DESCRIPTION_IOCTL "EnergyMon implementation for ODROID systems using ioctl")

# Libraries

add_library(${LNAME} ${SOURCES})
target_link_libraries(${LNAME} pthread rt)

add_library(${LNAME_IOCTL} ${SOURCES_IOCTL})
target_link_libraries(${LNAME_IOCTL} pthread rt)

if(DEFAULT STREQUAL "odroid" OR DEFAULT STREQUAL "energymon-odroid")
  BUILD_DEFAULT("${SOURCES}" "pthread;rt" "")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "" "-lpthread -lrt")
  install(TARGETS energymon-default DESTINATION lib)
elseif(DEFAULT STREQUAL "odroid-ioctl" OR DEFAULT STREQUAL "energymon-odroid-ioctl")
  BUILD_DEFAULT("${SOURCES_IOCTL}" "pthread;rt" "")
  PKG_CONFIG("energymon-default" "${DESCRIPTION_IOCTL}" "" "-lpthread -lrt")
  install(TARGETS energymon-default DESTINATION lib)
endif()

# pkg-config

PKG_CONFIG("${LNAME}" "${DESCRIPTION}" "" "-lpthread -lrt")
PKG_CONFIG("${LNAME_IOCTL}" "${DESCRIPTION_IOCTL}" "" "-lpthread -lrt")

# Install

install(TARGETS ${LNAME} ${LNAME_IOCTL} DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME}.h ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME_IOCTL}.h
        DESTINATION include/${PROJECT_NAME})
