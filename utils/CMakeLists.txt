# Utilities

function(CONFIGURE_MAN name prefix util)
  string(REPLACE "-" "\\-" MAN_BINARY_PREFIX "${prefix}")
  string(TOUPPER "${MAN_BINARY_PREFIX}" MAN_BINARY_PREFIX_UPPER)
  string(REPLACE "-" "\\-" MAN_IMPL "${name}")
  configure_file(
    ${CMAKE_SOURCE_DIR}/utils/man/energymon-${util}.1.in
    ${CMAKE_CURRENT_BINARY_DIR}/man/man1/${prefix}-${util}.1
    @ONLY
  )
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/man/man1/${prefix}-${util}.1
          DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
endfunction(CONFIGURE_MAN)

function(BUILD_UTILS name lib energymon_get_c)
  if(NOT ENERGYMON_BUILD_UTILITIES)
    return()
  endif()

  if(${name} STREQUAL "default")
    # special case - default implementation
    set(UTIL_PREFIX "energymon")
  else()
    set(UTIL_PREFIX "energymon-${name}")
  endif()

  add_executable(${UTIL_PREFIX}-cmd-profile ${CMAKE_SOURCE_DIR}/utils/energymon-cmd-profile.c
                                            ${energymon_get_c}
                                            ${ENERGYMON_TIME_UTIL})
  target_link_libraries(${UTIL_PREFIX}-cmd-profile ${lib} ${LIBRT})
  CONFIGURE_MAN(${name} ${UTIL_PREFIX} cmd-profile)

  add_executable(${UTIL_PREFIX}-file-provider ${CMAKE_SOURCE_DIR}/utils/energymon-file-provider.c
                                              ${energymon_get_c}
                                              ${ENERGYMON_TIME_UTIL})
  target_link_libraries(${UTIL_PREFIX}-file-provider ${lib} ${LIBRT})
  CONFIGURE_MAN(${name} ${UTIL_PREFIX} file-provider)

  add_executable(${UTIL_PREFIX}-idle-power ${CMAKE_SOURCE_DIR}/utils/energymon-idle-power.c
                                           ${energymon_get_c}
                                           ${ENERGYMON_TIME_UTIL})
  target_link_libraries(${UTIL_PREFIX}-idle-power ${lib} ${LIBRT})
  CONFIGURE_MAN(${name} ${UTIL_PREFIX} idle-power)

  add_executable(${UTIL_PREFIX}-info ${CMAKE_SOURCE_DIR}/utils/energymon-info.c
                                     ${energymon_get_c})
  target_link_libraries(${UTIL_PREFIX}-info ${lib})
  CONFIGURE_MAN(${name} ${UTIL_PREFIX} info)

  add_executable(${UTIL_PREFIX}-overhead ${CMAKE_SOURCE_DIR}/utils/energymon-overhead.c
                                         ${energymon_get_c}
                                         ${ENERGYMON_TIME_UTIL})
  target_link_libraries(${UTIL_PREFIX}-overhead ${lib} ${LIBRT})
  CONFIGURE_MAN(${name} ${UTIL_PREFIX} overhead)

  add_executable(${UTIL_PREFIX}-power-poller ${CMAKE_SOURCE_DIR}/utils/energymon-power-poller.c
                                             ${energymon_get_c}
                                             ${ENERGYMON_TIME_UTIL})
  target_link_libraries(${UTIL_PREFIX}-power-poller ${lib} ${LIBRT} ${LIBM})
  CONFIGURE_MAN(${name} ${UTIL_PREFIX} power-poller)

  install(TARGETS ${UTIL_PREFIX}-cmd-profile
                  ${UTIL_PREFIX}-file-provider
                  ${UTIL_PREFIX}-idle-power
                  ${UTIL_PREFIX}-info
                  ${UTIL_PREFIX}-overhead
                  ${UTIL_PREFIX}-power-poller
          DESTINATION ${CMAKE_INSTALL_BINDIR})
endfunction(BUILD_UTILS)
