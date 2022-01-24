# Utilities

function(configure_energymon_util_man SHORT_NAME UTIL_PREFIX UTIL_SUFFIX)
  string(REPLACE "-" "\\-" MAN_BINARY_PREFIX "${UTIL_PREFIX}")
  string(TOUPPER "${MAN_BINARY_PREFIX}" MAN_BINARY_PREFIX_UPPER)
  string(REPLACE "-" "\\-" MAN_IMPL "${SHORT_NAME}")
  configure_file(
    ${PROJECT_SOURCE_DIR}/utils/man/energymon-${UTIL_SUFFIX}.1.in
    ${CMAKE_CURRENT_BINARY_DIR}/man/man1/${UTIL_PREFIX}-${UTIL_SUFFIX}.1
    @ONLY
  )
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/man/man1/${UTIL_PREFIX}-${UTIL_SUFFIX}.1
          DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
endfunction(configure_energymon_util_man)

function(add_energymon_utils SHORT_NAME TARGET_LIB ENERGYMON_GET_C)
  if(NOT ENERGYMON_BUILD_UTILITIES)
    return()
  endif()

  if(${SHORT_NAME} STREQUAL "default")
    # special case - default implementation
    set(UTIL_PREFIX "energymon")
  else()
    set(UTIL_PREFIX "energymon-${SHORT_NAME}")
  endif()

  add_executable(${UTIL_PREFIX}-cmd-profile ${PROJECT_SOURCE_DIR}/utils/energymon-cmd-profile.c
                                            ${ENERGYMON_GET_C}
                                            ${ENERGYMON_TIME_UTIL})
  target_include_directories(${UTIL_PREFIX}-cmd-profile PRIVATE ${PROJECT_SOURCE_DIR}/common)
  target_compile_definitions(${UTIL_PREFIX}-cmd-profile PRIVATE ENERGYMON_UTIL_PREFIX=\"${UTIL_PREFIX}\")
  target_link_libraries(${UTIL_PREFIX}-cmd-profile PRIVATE ${TARGET_LIB} ${LIBRT})
  configure_energymon_util_man(${SHORT_NAME} ${UTIL_PREFIX} cmd-profile)

  add_executable(${UTIL_PREFIX}-file-provider ${PROJECT_SOURCE_DIR}/utils/energymon-file-provider.c
                                              ${ENERGYMON_GET_C}
                                              ${ENERGYMON_TIME_UTIL})
  target_include_directories(${UTIL_PREFIX}-file-provider PRIVATE ${PROJECT_SOURCE_DIR}/common)
  target_compile_definitions(${UTIL_PREFIX}-file-provider PRIVATE ENERGYMON_UTIL_PREFIX=\"${UTIL_PREFIX}\")
  target_link_libraries(${UTIL_PREFIX}-file-provider PRIVATE ${TARGET_LIB} ${LIBRT})
  configure_energymon_util_man(${SHORT_NAME} ${UTIL_PREFIX} file-provider)

  add_executable(${UTIL_PREFIX}-idle-power ${PROJECT_SOURCE_DIR}/utils/energymon-idle-power.c
                                           ${ENERGYMON_GET_C}
                                           ${ENERGYMON_TIME_UTIL})
  target_include_directories(${UTIL_PREFIX}-idle-power PRIVATE ${PROJECT_SOURCE_DIR}/common)
  target_compile_definitions(${UTIL_PREFIX}-idle-power PRIVATE ENERGYMON_UTIL_PREFIX=\"${UTIL_PREFIX}\")
  target_link_libraries(${UTIL_PREFIX}-idle-power PRIVATE ${TARGET_LIB} ${LIBRT})
  configure_energymon_util_man(${SHORT_NAME} ${UTIL_PREFIX} idle-power)

  add_executable(${UTIL_PREFIX}-info ${PROJECT_SOURCE_DIR}/utils/energymon-info.c
                                     ${ENERGYMON_GET_C})
  target_include_directories(${UTIL_PREFIX}-info PRIVATE ${PROJECT_SOURCE_DIR}/common)
  target_compile_definitions(${UTIL_PREFIX}-info PRIVATE ENERGYMON_UTIL_PREFIX=\"${UTIL_PREFIX}\")
  target_link_libraries(${UTIL_PREFIX}-info PRIVATE ${TARGET_LIB})
  configure_energymon_util_man(${SHORT_NAME} ${UTIL_PREFIX} info)

  add_executable(${UTIL_PREFIX}-overhead ${PROJECT_SOURCE_DIR}/utils/energymon-overhead.c
                                         ${ENERGYMON_GET_C}
                                         ${ENERGYMON_TIME_UTIL})
  target_include_directories(${UTIL_PREFIX}-overhead PRIVATE ${PROJECT_SOURCE_DIR}/common)
  target_compile_definitions(${UTIL_PREFIX}-overhead PRIVATE ENERGYMON_UTIL_PREFIX=\"${UTIL_PREFIX}\")
  target_link_libraries(${UTIL_PREFIX}-overhead PRIVATE ${TARGET_LIB} ${LIBRT})
  configure_energymon_util_man(${SHORT_NAME} ${UTIL_PREFIX} overhead)

  add_executable(${UTIL_PREFIX}-power-poller ${PROJECT_SOURCE_DIR}/utils/energymon-power-poller.c
                                             ${ENERGYMON_GET_C}
                                             ${ENERGYMON_TIME_UTIL})
  target_include_directories(${UTIL_PREFIX}-power-poller PRIVATE ${PROJECT_SOURCE_DIR}/common)
  target_compile_definitions(${UTIL_PREFIX}-power-poller PRIVATE ENERGYMON_UTIL_PREFIX=\"${UTIL_PREFIX}\")
  target_link_libraries(${UTIL_PREFIX}-power-poller PRIVATE ${TARGET_LIB} ${LIBRT} ${LIBM})
  configure_energymon_util_man(${SHORT_NAME} ${UTIL_PREFIX} power-poller)

  install(TARGETS ${UTIL_PREFIX}-cmd-profile
                  ${UTIL_PREFIX}-file-provider
                  ${UTIL_PREFIX}-idle-power
                  ${UTIL_PREFIX}-info
                  ${UTIL_PREFIX}-overhead
                  ${UTIL_PREFIX}-power-poller
          EXPORT EnergyMonTargets
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endfunction(add_energymon_utils)
