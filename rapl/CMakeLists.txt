set(LNAME energymon-rapl)
set(SOURCES ${LNAME}.c;../src/energymon-util.c)

# Libraries

add_library(${LNAME} SHARED ${SOURCES})
SET_FPIC(${LNAME})

add_library(${LNAME}-static STATIC ${SOURCES})
SET_FPIC(${LNAME}-static)

if(DEFAULT STREQUAL "rapl")
  BUILD_DEFAULT("${SOURCES}" "" "")
endif()

# pkg-config

PKG_CONFIG("${LNAME}" "EnergyMon implementation for Intel RAPL" "")
PKG_CONFIG("${LNAME}-static" "EnergyMon implementation for Intel RAPL" "")

# Install

install(TARGETS ${LNAME} ${LNAME}-static DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME}.h DESTINATION include/${PROJECT_NAME})
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/ DESTINATION lib/pkgconfig)
