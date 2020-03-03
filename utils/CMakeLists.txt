# Utilities

macro(BUILD_UTILS name lib get_header get_function)
  if(NOT ENERGYMON_BUILD_UTILITIES)
    return()
  endif()

  set(ENERGYMON_GET_HEADER ${get_header})
  set(ENERGYMON_GET_FUNCTION ${get_function})
  configure_file(
    ${ENERGYMON_GET_HEADER_IN}
    ${CMAKE_CURRENT_BINARY_DIR}/${name}/energymon-get.h
    @ONLY
  )

  add_executable(${name}-cmd-profile ${CMAKE_SOURCE_DIR}/utils/energymon-cmd-profile.c;${ENERGYMON_TIME_UTIL})
  target_include_directories(${name}-cmd-profile PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/${name})
  target_link_libraries(${name}-cmd-profile ${lib} ${LIBRT})

  add_executable(${name}-file-provider ${CMAKE_SOURCE_DIR}/utils/energymon-file-provider.c;${ENERGYMON_TIME_UTIL})
  target_include_directories(${name}-file-provider PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/${name})
  target_link_libraries(${name}-file-provider ${lib} ${LIBRT})

  add_executable(${name}-idle-power ${CMAKE_SOURCE_DIR}/utils/energymon-idle-power.c;${ENERGYMON_TIME_UTIL})
  target_include_directories(${name}-idle-power PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/${name})
  target_link_libraries(${name}-idle-power ${lib} ${LIBRT})

  add_executable(${name}-info ${CMAKE_SOURCE_DIR}/utils/energymon-info.c)
  target_include_directories(${name}-info PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/${name})
  target_link_libraries(${name}-info ${lib})

  add_executable(${name}-overhead ${CMAKE_SOURCE_DIR}/utils/energymon-overhead.c;${ENERGYMON_TIME_UTIL})
  target_include_directories(${name}-overhead PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/${name})
  target_link_libraries(${name}-overhead ${lib} ${LIBRT})

  add_executable(${name}-power-poller ${CMAKE_SOURCE_DIR}/utils/energymon-power-poller.c;${ENERGYMON_TIME_UTIL})
  target_include_directories(${name}-power-poller PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/${name})
  target_link_libraries(${name}-power-poller ${lib} ${LIBRT} ${LIBM})

  install(TARGETS ${name}-cmd-profile
                  ${name}-file-provider
                  ${name}-idle-power
                  ${name}-info
                  ${name}-overhead
                  ${name}-power-poller
          DESTINATION ${CMAKE_INSTALL_BINDIR})
endmacro(BUILD_UTILS)

if(ENERGYMON_BUILD_UTILITIES)
  install(DIRECTORY man/ DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
endif()
