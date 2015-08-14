set(LNAME energymon-rapl)

# Libraries

add_library(${LNAME} SHARED ${LNAME}.c)
SET_FPIC(${LNAME})

add_library(${LNAME}-static STATIC ${LNAME}.c)
SET_FPIC(${LNAME}-static)

if(DEFAULT STREQUAL "rapl")
  BUILD_DEFAULT("${LNAME}.c" "" "")
endif()

# pkg-config

PKG_CONFIG("${LNAME}" "EnergyMon implementation for Intel RAPL" "")
PKG_CONFIG("${LNAME}-static" "EnergyMon implementation for Intel RAPL" "")

# Install

install(TARGETS ${LNAME} ${LNAME}-static DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME}.h DESTINATION include/${PROJECT_NAME})
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/ DESTINATION lib/pkgconfig)
