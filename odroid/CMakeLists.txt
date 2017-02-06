set(LNAME energymon-odroid)
set(LNAME_IOCTL energymon-odroid-ioctl)
set(SOURCES ${LNAME}.c;${ENERGYMON_UTIL};${ENERGYMON_TIME_UTIL})
set(SOURCES_IOCTL ${LNAME_IOCTL}.c;${ENERGYMON_UTIL};${ENERGYMON_TIME_UTIL})
set(DESCRIPTION "EnergyMon implementation for ODROID systems")
set(DESCRIPTION_IOCTL "EnergyMon implementation for ODROID systems using ioctl")

# Dependencies

# Must be set to OFF to prevent doing a try_run() during cross-compiling
set(THREADS_PTHREAD_ARG OFF)
find_package(Threads)
if(CMAKE_THREAD_LIBS_INIT)
  set(PKG_CONFIG_PRIVATE_LIBS "${CMAKE_THREAD_LIBS_INIT}")
endif()
if(LIBRT)
  set(PKG_CONFIG_PRIVATE_LIBS "${PKG_CONFIG_PRIVATE_LIBS} -lrt")
endif()

# Libraries

add_library(${LNAME} ${SOURCES})
target_link_libraries(${LNAME} ${CMAKE_THREAD_LIBS_INIT} ${LIBRT})

add_library(${LNAME_IOCTL} ${SOURCES_IOCTL})
target_link_libraries(${LNAME_IOCTL} ${CMAKE_THREAD_LIBS_INIT} ${LIBRT})

if(DEFAULT STREQUAL "odroid" OR DEFAULT STREQUAL LNAME)
  BUILD_DEFAULT("${SOURCES}" "${CMAKE_THREAD_LIBS_INIT};${LIBRT}" "")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "" "${PKG_CONFIG_PRIVATE_LIBS}")
  install(TARGETS energymon-default DESTINATION lib)
elseif(DEFAULT STREQUAL "odroid-ioctl" OR DEFAULT STREQUAL LNAME_IOCTL)
  BUILD_DEFAULT("${SOURCES_IOCTL}" "${CMAKE_THREAD_LIBS_INIT};${LIBRT}" "")
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
