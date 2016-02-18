set(LNAME energymon-wattsup)
set(SHMEM_PROVIDER ${LNAME}-shmem-provider)
set(SOURCES ${LNAME}.c;../src/energymon-util.c;../src/energymon-time-util.c)
set(DESCRIPTION "EnergyMon implementation for WattsUp? Power meters")

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

if(DEFAULT STREQUAL "wattsup" OR DEFAULT STREQUAL LNAME)
  BUILD_DEFAULT("${SOURCES}" "${CMAKE_THREAD_LIBS_INIT};${LIBRT}" "")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "" "${PKG_CONFIG_PRIVATE_LIBS}")
  install(TARGETS energymon-default DESTINATION lib)
endif()

# pkg-config

PKG_CONFIG("${LNAME}" "${DESCRIPTION}" "" "${PKG_CONFIG_PRIVATE_LIBS}")

# Binaries

add_executable(${SHMEM_PROVIDER} energymon-wattsup-shmem-provider.c)
target_link_libraries(${SHMEM_PROVIDER} ${LNAME})

# Install

install(TARGETS ${LNAME} DESTINATION lib)
install(TARGETS ${SHMEM_PROVIDER} DESTINATION bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME}.h DESTINATION include/${PROJECT_NAME})
