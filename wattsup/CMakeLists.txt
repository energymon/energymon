# Dependencies

# Must be set to OFF to prevent doing a try_run() during cross-compiling
set(THREADS_PTHREAD_ARG OFF)
find_package(Threads)
if(CMAKE_THREAD_LIBS_INIT)
  set(PKG_CONFIG_PRIVATE_LIBS "${CMAKE_THREAD_LIBS_INIT}")
endif()
if(LIBRT)
  set(PKG_CONFIG_PRIVATE_LIBS "${PKG_CONFIG_PRIVATE_LIBS} -lrt")
endif()

# Libraries

if(UNIX)
  add_subdirectory(dev)
endif()
if(UNIX OR WIN32)
  add_subdirectory(libftdi)
  add_subdirectory(libusb)
endif()

# Install

# Don't install headers if no implementation is built
if(TARGET energymon-wattsup OR TARGET energymon-wattsup-libftdi OR TARGET energymon-wattsup-libusb)
  install(FILES energymon-wattsup.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
endif()
