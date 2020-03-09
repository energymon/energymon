# Utilities

function(CONFIGURE_MAN name util)
  string(REPLACE "-" "\\-" MAN_BINARY_PREFIX "${name}")
  string(TOUPPER "${MAN_BINARY_PREFIX}" MAN_BINARY_PREFIX_UPPER)
  if(${name} STREQUAL "energymon")
    # special case - default implementation
    set(MAN_IMPL "default")
  else()
    set(MAN_IMPL ${name})
  endif()
  configure_file(
    ${CMAKE_SOURCE_DIR}/utils/man/energymon-${util}.1.in
    ${CMAKE_CURRENT_BINARY_DIR}/man/man1/${name}-${util}.1
    @ONLY
  )
endfunction(CONFIGURE_MAN)

macro(BUILD_UTILS name lib energymon_get_c)
  if(NOT ENERGYMON_BUILD_UTILITIES)
    return()
  endif()

  add_executable(${name}-cmd-profile ${CMAKE_SOURCE_DIR}/utils/energymon-cmd-profile.c
                                     ${energymon_get_c}
                                     ${ENERGYMON_TIME_UTIL})
  target_link_libraries(${name}-cmd-profile ${lib} ${LIBRT})
  CONFIGURE_MAN(${name} cmd-profile)

  add_executable(${name}-file-provider ${CMAKE_SOURCE_DIR}/utils/energymon-file-provider.c
                                       ${energymon_get_c}
                                       ${ENERGYMON_TIME_UTIL})
  target_link_libraries(${name}-file-provider ${lib} ${LIBRT})
  CONFIGURE_MAN(${name} file-provider)

  add_executable(${name}-idle-power ${CMAKE_SOURCE_DIR}/utils/energymon-idle-power.c
                                    ${energymon_get_c}
                                    ${ENERGYMON_TIME_UTIL})
  target_link_libraries(${name}-idle-power ${lib} ${LIBRT})
  CONFIGURE_MAN(${name} idle-power)

  add_executable(${name}-info ${CMAKE_SOURCE_DIR}/utils/energymon-info.c
                              ${energymon_get_c})
  target_link_libraries(${name}-info ${lib})
  CONFIGURE_MAN(${name} info)

  add_executable(${name}-overhead ${CMAKE_SOURCE_DIR}/utils/energymon-overhead.c
                                  ${energymon_get_c}
                                  ${ENERGYMON_TIME_UTIL})
  target_link_libraries(${name}-overhead ${lib} ${LIBRT})
  CONFIGURE_MAN(${name} overhead)

  add_executable(${name}-power-poller ${CMAKE_SOURCE_DIR}/utils/energymon-power-poller.c
                                      ${energymon_get_c}
                                      ${ENERGYMON_TIME_UTIL})
  target_link_libraries(${name}-power-poller ${lib} ${LIBRT} ${LIBM})
  CONFIGURE_MAN(${name} power-poller)

  install(TARGETS ${name}-cmd-profile
                  ${name}-file-provider
                  ${name}-idle-power
                  ${name}-info
                  ${name}-overhead
                  ${name}-power-poller
          DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/man/ DESTINATION ${CMAKE_INSTALL_MANDIR})
endmacro(BUILD_UTILS)
