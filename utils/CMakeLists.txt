# Utilities

add_executable(energymon-info energymon-info.c)
target_link_libraries(energymon-info energymon-default)
install(TARGETS energymon-info DESTINATION bin)

if(UNIX)
  add_executable(energymon-cmd-profile energymon-cmd-profile.c;${ENERGYMON_TIME_UTIL})
  target_link_libraries(energymon-cmd-profile energymon-default ${LIBRT})

  add_executable(energymon-file-provider energymon-file-provider.c;${ENERGYMON_TIME_UTIL})
  target_link_libraries(energymon-file-provider energymon-default ${LIBRT})

  add_executable(energymon-idle-power energymon-idle-power.c;${ENERGYMON_TIME_UTIL})
  target_link_libraries(energymon-idle-power energymon-default ${LIBRT})

  add_executable(energymon-overhead energymon-overhead.c;${ENERGYMON_TIME_UTIL})
  target_link_libraries(energymon-overhead energymon-default ${LIBRT})

  add_executable(energymon-power-poller energymon-power-poller.c;${ENERGYMON_TIME_UTIL})
  target_link_libraries(energymon-power-poller energymon-default ${LIBRT} m)

  install(TARGETS energymon-cmd-profile energymon-file-provider energymon-idle-power energymon-overhead energymon-power-poller DESTINATION bin)
endif()
