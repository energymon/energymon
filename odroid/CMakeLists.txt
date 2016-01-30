set(LNAME energymon-odroid)
set(LNAME_IOCTL energymon-odroid-ioctl)
set(SOURCES ${LNAME}.c;../src/energymon-util.c;../src/energymon-time-util.c)
set(SOURCES_IOCTL ${LNAME_IOCTL}.c;../src/energymon-util.c;../src/energymon-time-util.c)
set(DESCRIPTION "EnergyMon implementation for ODROID systems")
set(DESCRIPTION_IOCTL "EnergyMon implementation for ODROID systems using ioctl")

# Libraries

add_library(${LNAME} ${SOURCES})
target_link_libraries(${LNAME} pthread ${LIBRT})

add_library(${LNAME_IOCTL} ${SOURCES_IOCTL})
target_link_libraries(${LNAME_IOCTL} pthread ${LIBRT})

if(LIBRT)
  set(PKG_CONFIG_PRIVATE_LIBS "-lpthread -lrt")
else()
  set(PKG_CONFIG_PRIVATE_LIBS "-lpthread")
endif()

if(DEFAULT STREQUAL "odroid" OR DEFAULT STREQUAL LNAME)
  BUILD_DEFAULT("${SOURCES}" "pthread;${LIBRT}" "")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "" "${PKG_CONFIG_PRIVATE_LIBS}")
  install(TARGETS energymon-default DESTINATION lib)
elseif(DEFAULT STREQUAL "odroid-ioctl" OR DEFAULT STREQUAL LNAME_IOCTL)
  BUILD_DEFAULT("${SOURCES_IOCTL}" "pthread;${LIBRT}" "")
  PKG_CONFIG("energymon-default" "${DESCRIPTION_IOCTL}" "" "${PKG_CONFIG_PRIVATE_LIBS}")
  install(TARGETS energymon-default DESTINATION lib)
endif()

# pkg-config

PKG_CONFIG("${LNAME}" "${DESCRIPTION}" "" "${PKG_CONFIG_PRIVATE_LIBS}")
PKG_CONFIG("${LNAME_IOCTL}" "${DESCRIPTION_IOCTL}" "" "${PKG_CONFIG_PRIVATE_LIBS}")

# Install

install(TARGETS ${LNAME} ${LNAME_IOCTL} DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME}.h ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME_IOCTL}.h
        DESTINATION include/${PROJECT_NAME})
