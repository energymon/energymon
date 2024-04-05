# Dependencies

find_package(Threads)
if(CMAKE_THREAD_LIBS_INIT)
  list(APPEND PKG_CONFIG_PRIVATE_LIBS "${CMAKE_THREAD_LIBS_INIT}")
endif()

# Libraries

add_subdirectory(dev)
add_subdirectory(libftdi)
add_subdirectory(libusb)
