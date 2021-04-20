# Dependencies

find_package(Threads)
if(CMAKE_THREAD_LIBS_INIT)
  list(APPEND PKG_CONFIG_PRIVATE_LIBS "${CMAKE_THREAD_LIBS_INIT}")
endif()

if(LIBRT)
  list(APPEND PKG_CONFIG_PRIVATE_LIBS "-lrt")
endif()

# Libraries

if(UNIX)
  add_subdirectory(dev)
endif()
if(UNIX OR WIN32)
  add_subdirectory(libftdi)
  add_subdirectory(libusb)
endif()
