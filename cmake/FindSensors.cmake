# Find the sensors include file and library from lm-sensors.
#
# This module defines the following IMPORTED target(s):
#
# Sensors::Sensors
# The sensors library, if found.

find_library(Sensors_LIBRARY sensors)
find_path(
  Sensors_INCLUDE_DIR
  NAMES sensors.h
  PATH_SUFFIXES include sensors
)
mark_as_advanced(
  Sensors_LIBRARY
  Sensors_INCLUDE_DIR
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Sensors
  REQUIRED_VARS Sensors_LIBRARY
                Sensors_INCLUDE_DIR
  VERSION_VAR Sensors_VERSION
)

if(Sensors_FOUND)
  add_library(Sensors::Sensors UNKNOWN IMPORTED)
  set_target_properties(Sensors::Sensors
    PROPERTIES
      IMPORTED_LOCATION "${Sensors_LIBRARY}"
      IMPORTED_LINK_INTERFACE_LANGUAGES "C"
      INTERFACE_INCLUDE_DIRECTORIES "${Sensors_INCLUDE_DIR}"
  )
endif()
