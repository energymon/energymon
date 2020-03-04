# Utilities

macro(BUILD_UTILS name lib energymon_get_c)
  if(NOT ENERGYMON_BUILD_UTILITIES)
    return()
  endif()

  add_executable(${name}-cmd-profile ${CMAKE_SOURCE_DIR}/utils/energymon-cmd-profile.c
                                     ${energymon_get_c}
                                     ${ENERGYMON_TIME_UTIL})
  target_link_libraries(${name}-cmd-profile ${lib} ${LIBRT})

  add_executable(${name}-file-provider ${CMAKE_SOURCE_DIR}/utils/energymon-file-provider.c
                                       ${energymon_get_c}
                                       ${ENERGYMON_TIME_UTIL})
  target_link_libraries(${name}-file-provider ${lib} ${LIBRT})

  add_executable(${name}-idle-power ${CMAKE_SOURCE_DIR}/utils/energymon-idle-power.c
                                    ${energymon_get_c}
                                    ${ENERGYMON_TIME_UTIL})
  target_link_libraries(${name}-idle-power ${lib} ${LIBRT})

  add_executable(${name}-info ${CMAKE_SOURCE_DIR}/utils/energymon-info.c
                              ${energymon_get_c})
  target_link_libraries(${name}-info ${lib})

  add_executable(${name}-overhead ${CMAKE_SOURCE_DIR}/utils/energymon-overhead.c
                                  ${energymon_get_c}
                                  ${ENERGYMON_TIME_UTIL})
  target_link_libraries(${name}-overhead ${lib} ${LIBRT})

  add_executable(${name}-power-poller ${CMAKE_SOURCE_DIR}/utils/energymon-power-poller.c
                                      ${energymon_get_c}
                                      ${ENERGYMON_TIME_UTIL})
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
