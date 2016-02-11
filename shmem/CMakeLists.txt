set(LNAME energymon-shmem)
set(EXAMPLE energymon-shmem-example)
set(SOURCES ${LNAME}.c;../src/energymon-util.c)
set(DESCRIPTION "EnergyMon over Shared Memory")

# Libraries

add_library(${LNAME} ${SOURCES})

if(DEFAULT STREQUAL "shmem" OR DEFAULT STREQUAL LNAME)
  BUILD_DEFAULT("${SOURCES}" "" "")
  PKG_CONFIG("energymon-default" "${DESCRIPTION}" "" "")
  install(TARGETS energymon-default DESTINATION lib)
endif()

# Binaries

add_executable(${EXAMPLE} example/${EXAMPLE}.c)
target_link_libraries(${EXAMPLE} energymon-default)

# pkg-config

PKG_CONFIG("${LNAME}" "${DESCRIPTION}" "" "")

# Install

install(TARGETS ${LNAME} DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME}.h DESTINATION include/${PROJECT_NAME})
