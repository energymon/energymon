set(LNAME energymon-msr)

# Libraries

add_library(${LNAME} SHARED ${LNAME}.c)
target_link_libraries(${LNAME} m)
SET_FPIC(${LNAME})

add_library(${LNAME}-static STATIC ${LNAME}.c)
target_link_libraries(${LNAME}-static m)
SET_FPIC(${LNAME}-static)

if(DEFAULT STREQUAL "msr")
  BUILD_DEFAULT("${LNAME}.c" "m" "")
endif()

# pkg-config

PKG_CONFIG("${LNAME}" "EnergyMon implementation for Intel Model Specific Register" "-lm")
PKG_CONFIG("${LNAME}-static" "EnergyMon implementation for Intel Model Specific Register" "-lm")

# Install

install(TARGETS ${LNAME} ${LNAME}-static DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME}.h DESTINATION include/${PROJECT_NAME})
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/ DESTINATION lib/pkgconfig)
