set(LNAME energymon-odroid)
set(SOURCES ${LNAME}.c;../src/energymon-util.c)

# Libraries

add_library(${LNAME} SHARED ${SOURCES})
target_link_libraries(${LNAME} pthread)
SET_FPIC(${LNAME})

add_library(${LNAME}-static STATIC ${SOURCES})
target_link_libraries(${LNAME}-static pthread)
SET_FPIC(${LNAME}-static)

if(DEFAULT STREQUAL "odroid")
  BUILD_DEFAULT("${SOURCES}" "pthread" "")
endif()

# pkg-config

PKG_CONFIG("${LNAME}" "EnergyMon implementation for ODROID systems" "-lpthread")
PKG_CONFIG("${LNAME}-static" "EnergyMon implementation for ODROID systems" "-lpthread")

# Install

install(TARGETS ${LNAME} ${LNAME}-static DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${LNAME}.h DESTINATION include/${PROJECT_NAME})
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/ DESTINATION lib/pkgconfig)
