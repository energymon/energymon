set(LNAME energymon-msr)
set(SOURCES ${LNAME}.c;../src/energymon-util.c)

# Libraries

add_library(${LNAME} SHARED ${SOURCES})
target_link_libraries(${LNAME} m)
SET_FPIC(${LNAME})

add_library(${LNAME}-static STATIC ${SOURCES})
target_link_libraries(${LNAME}-static m)
SET_FPIC(${LNAME}-static)

if(DEFAULT STREQUAL "msr")
  BUILD_DEFAULT("${SOURCES}" "m" "")
endif()

# pkg-config

PKG_CONFIG("${LNAME}" "EnergyMon implementation for Intel Model Specific Register" "-lm")
PKG_CONFIG("${LNAME}-static" "EnergyMon implementation for Intel Model Specific Register" "-lm")

# Install

install(TARGETS ${LNAME} ${LNAME}-static DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME}.h DESTINATION include/${PROJECT_NAME})
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/ DESTINATION lib/pkgconfig)
