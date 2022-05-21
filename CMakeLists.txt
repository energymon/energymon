cmake_minimum_required(VERSION 3.6)

project(energymon VERSION 0.5.1
                  LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
add_compile_options(-Wall)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(GNUInstallDirs)

set(ENERGYMON_UTIL ${PROJECT_SOURCE_DIR}/common/energymon-util.c)
set(ENERGYMON_TIME_UTIL ${PROJECT_SOURCE_DIR}/common/energymon-time-util.c;${PROJECT_SOURCE_DIR}/common/ptime/ptime.c)

if(UNIX AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  # Determine if we should link with librt for libraries that use "clock_gettime"
  include(CheckFunctionExists)
  CHECK_FUNCTION_EXISTS(clock_gettime HAVE_CLOCK_GETTIME)
  if(NOT HAVE_CLOCK_GETTIME)
    find_library(LIBRT NAMES rt)
  endif()
endif()

if(UNIX)
  set(LIBM m)
endif()


# Configurable build options

set(ENERGYMON_BUILD_LIB "ALL" CACHE STRING "Specify a single library to build, ALL, or NONE")
set(ENERGYMON_BUILD_DEFAULT "dummy" CACHE STRING "Default EnergyMon implementation, or NONE to disable building energymon-default")
if (DEFAULT AND NOT ENERGYMON_BUILD_DEFAULT)
  # for backward compatibility with old variable "DEFAULT"
  set(ENERGYMON_BUILD_DEFAULT DEFAULT)
endif()
set(ENERGYMON_BUILD_SHMEM_PROVIDERS TRUE CACHE BOOL "Enable/disable building shared memory providers")
set(ENERGYMON_BUILD_UTILITIES TRUE CACHE BOOL "Enable/disable building utility applications")
set(ENERGYMON_BUILD_TESTS TRUE CACHE BOOL "Enable/disable building tests")
set(ENERGYMON_BUILD_EXAMPLES TRUE CACHE BOOL "Enable/disable building example code")

set(ENERGYMON_INSTALL_CMAKE_PACKAGES FALSE CACHE BOOL "[Experimental] Enable/disable installing cmake package configuration files")


# Utilities

add_subdirectory(utils)
add_subdirectory(test)


# Functions

function(add_energymon_library TARGET SHORT_NAME)
  # Parse arguments
  set(options BUILD_SHMEM_PROVIDER)
  set(oneValueArgs ENERGYMON_GET_HEADER
                   ENERGYMON_GET_FUNCTION
                   ENERGYMON_GET_C_OUTPUT)
  set(multiValueArgs SOURCES
                     PUBLIC_HEADER
                     PUBLIC_BUILD_INCLUDE_DIRS)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  if(NOT "${ARG_UNPARSED_ARGUMENTS}" STREQUAL "")
      message(FATAL_ERROR "add_energymon_library: unrecognized args: ${ARG_UNPARSED_ARGUMENTS}")
  endif()

  # Create library
  add_library(${TARGET} ${ARG_SOURCES})
  target_include_directories(${TARGET} PRIVATE ${PROJECT_SOURCE_DIR}/common
                                       PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/inc$<SEMICOLON>${ARG_PUBLIC_BUILD_INCLUDE_DIRS}>
                                              $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}>)
  set_target_properties(${TARGET} PROPERTIES PUBLIC_HEADER "${PROJECT_SOURCE_DIR}/inc/energymon.h;${ARG_PUBLIC_HEADER}")
  if(BUILD_SHARED_LIBS)
    set_target_properties(${TARGET} PROPERTIES VERSION ${PROJECT_VERSION}
                                               SOVERSION ${PROJECT_VERSION_MAJOR})
  endif()
  install(TARGETS ${TARGET}
          EXPORT EnergyMonTargets
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
          PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

  # Tests and Utils
  set(ENERGYMON_GET_HEADER ${ARG_ENERGYMON_GET_HEADER})
  set(ENERGYMON_GET_FUNCTION ${ARG_ENERGYMON_GET_FUNCTION})
  configure_file(${PROJECT_SOURCE_DIR}/common/energymon-get.c.in ${ARG_ENERGYMON_GET_C_OUTPUT})
  add_energymon_tests(${SHORT_NAME} ${TARGET} ${ARG_ENERGYMON_GET_C_OUTPUT})
  add_energymon_utils(${SHORT_NAME} ${TARGET} ${ARG_ENERGYMON_GET_C_OUTPUT})

  # shmem provider
  if(ARG_BUILD_SHMEM_PROVIDER)
    add_energymon_shmem_provider(${SHORT_NAME} ${TARGET} ${ARG_ENERGYMON_GET_C_OUTPUT})
  endif()
endfunction()

# Forwards through to add_energymon_library, but the only argument should be SOURCES
function(add_energymon_default_library)
  add_energymon_library(energymon-default default ${ARGV}
                        PUBLIC_HEADER ${PROJECT_SOURCE_DIR}/inc/energymon-default.h
                        ENERGYMON_GET_HEADER energymon-default.h
                        ENERGYMON_GET_FUNCTION "energymon_get_default"
                        ENERGYMON_GET_C_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/energymon-default/energymon-get.c)
  target_compile_definitions(energymon-default PRIVATE "ENERGYMON_DEFAULT")
endfunction()

function(add_energymon_pkg_config TARGET DESCRIPTION REQUIRES_PRIVATE LIBS_PRIVATE)
  set(PKG_CONFIG_PREFIX "${CMAKE_INSTALL_PREFIX}")
  set(PKG_CONFIG_EXEC_PREFIX "\${prefix}")
  if(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
    set(PKG_CONFIG_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
  else()
    set(PKG_CONFIG_LIBDIR "\${exec_prefix}/${CMAKE_INSTALL_LIBDIR}")
  endif()
  if(IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}")
    set(PKG_CONFIG_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}")
  else()
    set(PKG_CONFIG_INCLUDEDIR "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}")
  endif()
  set(PKG_CONFIG_NAME "${TARGET}")
  set(PKG_CONFIG_DESCRIPTION "${DESCRIPTION}")
  string(REPLACE ";" " " PKG_CONFIG_REQUIRES_PRIVATE "${REQUIRES_PRIVATE}")
  set(PKG_CONFIG_CFLAGS "-I\${includedir}")
  set(PKG_CONFIG_LIBS "-L\${libdir} -l${TARGET}")
  string(REPLACE ";" " " PKG_CONFIG_LIBS_PRIVATE "${LIBS_PRIVATE}")
  configure_file(
    ${PROJECT_SOURCE_DIR}/pkgconfig.in
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.pc
    @ONLY
  )
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endfunction()

function(energymon_export_dependency PACKAGE)
  if(BUILD_SHARED_LIBS)
    return()
  endif()
  set_property(GLOBAL APPEND PROPERTY ENERGYMON_CMAKE_PACKAGE_DEPENDENCIES "find_dependency(${PACKAGE})")
endfunction()

function(energymon_export_pkg_dependency PREFIX MODULE_SPEC)
  # Parse arguments
  set(options IMPORTED_TARGET)
  set(oneValueArgs)
  set(multiValueArgs)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  if(NOT "${ARG_UNPARSED_ARGUMENTS}" STREQUAL "")
    message(FATAL_ERROR "energymon_export_pkg_dependency: unrecognized args: ${ARG_UNPARSED_ARGUMENTS}")
  endif()
  if(BUILD_SHARED_LIBS)
    return()
  endif()
  set_property(GLOBAL APPEND PROPERTY ENERGYMON_CMAKE_PACKAGE_DEPENDENCIES "find_pkg_config_dependency(${PREFIX} ${ARG_IMPORTED_TARGET} ${MODULE_SPEC})")
endfunction()


# Libraries

add_subdirectory(dummy)

if(UNIX)
  add_subdirectory(shmem)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux|Android")
  add_subdirectory(cray-pm)
  add_subdirectory(ibmpowernv)
  add_subdirectory(jetson)
  add_subdirectory(msr)
  add_subdirectory(odroid)
  add_subdirectory(rapl)
  add_subdirectory(raplcap-msr)
  add_subdirectory(zcu102)
endif()

if(UNIX OR WIN32)
  add_subdirectory(osp)
  add_subdirectory(wattsup)
endif()

if(APPLE)
  add_subdirectory(ipg)
endif()

if(NOT TARGET energymon-default AND NOT ${ENERGYMON_BUILD_DEFAULT} MATCHES "NONE")
  message(FATAL_ERROR
          "No build target for ENERGYMON_BUILD_DEFAULT=${ENERGYMON_BUILD_DEFAULT}\n"
          "Specified implementation may be unknown, unsupported for the target system, or skipped due to missing build dependencies\n"
          "Retry with a supported value or set ENERGYMON_BUILD_DEFAULT=NONE")
endif()


# CMake package helpers

if (ENERGYMON_INSTALL_CMAKE_PACKAGES)
  include(CMakePackageConfigHelpers)
  set(ENERGYMON_CMAKE_CONFIG_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/energymon)
  get_property(PACKAGE_CONFIG_REQUIRED_DEPENDENCIES GLOBAL PROPERTY ENERGYMON_CMAKE_PACKAGE_DEPENDENCIES)
  list(REMOVE_DUPLICATES PACKAGE_CONFIG_REQUIRED_DEPENDENCIES)
  string(REPLACE ";" "\n" CONFIG_FIND_DEPENDENCIES "${PACKAGE_CONFIG_REQUIRED_DEPENDENCIES}")
  set(CONFIG_TARGETS_FILE EnergyMonTargets.cmake)
  configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/Config.cmake.in
    ${CMAKE_BINARY_DIR}/EnergyMonConfig.cmake
    INSTALL_DESTINATION ${ENERGYMON_CMAKE_CONFIG_DIR}
  )
  write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/EnergyMonConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )
  install(FILES ${CMAKE_BINARY_DIR}/EnergyMonConfig.cmake
                ${CMAKE_BINARY_DIR}/EnergyMonConfigVersion.cmake
          DESTINATION ${ENERGYMON_CMAKE_CONFIG_DIR})
  if(NOT BUILD_SHARED_LIBS)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cmake/
            DESTINATION ${ENERGYMON_CMAKE_CONFIG_DIR}/Modules
            FILES_MATCHING PATTERN "Find*.cmake")
  endif()
  install(EXPORT EnergyMonTargets
          DESTINATION ${ENERGYMON_CMAKE_CONFIG_DIR}
          NAMESPACE EnergyMon::)
endif()


# Uninstall

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
  @ONLY
)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
