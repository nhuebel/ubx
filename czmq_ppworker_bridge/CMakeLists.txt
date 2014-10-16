CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(czmq_ppworker_bridge)

set(CMAKE_CXX_FLAGS "-Wall -Werror -fvisibility=hidden")
set(CMAKE_CXX_COMPILER clang++ )
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake") ## temporary resources, for UBX

# Global
set(INSTALL_LIB_TYPES_DIR lib/ubx/types CACHE PATH "Installation directory for types libraries")
set(INSTALL_LIB_BLOCKS_DIR lib/ubx/blocks CACHE PATH "Installation directory for blocks libraries")
set(INSTALL_BIN_APPS_DIR bin CACHE PATH "Installation directory for bin")
set(INSTALL_INCLUDE_DIR include/ubx CACHE PATH "Installation directory for header files (types)")
set(INSTALL_CMAKE_DIR  share/ubx/cmake CACHE PATH  "Installation directory for CMake files") # default ${DEF_INSTALL_CMAKE_DIR}

FIND_PACKAGE(CZMQ REQUIRED)

# Make relative paths absolute
foreach(p LIB_TYPES LIB_BLOCKS BIN INCLUDE CMAKE)
  set(var INSTALL_${p}_DIR)
  if(NOT IS_ABSOLUTE "${${var}}")
    set(${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
  endif()
endforeach()

##
# Add uninstall target.
##
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)
    
add_custom_target(uninstall
    "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
)

##
# Generate config package
##
set(CONF_INCLUDE_DIRS "${INSTALL_INCLUDE_DIR}")
set(CONF_CMAKE_DIR "${INSTALL_CMAKE_DIR}")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/czmq-ppworker-bridge-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/czmq-ppworker-bridge-config.cmake" @ONLY
)

set (UBX_ROOT $ENV{UBX_ROOT} )
if (NOT UBX_ROOT)
 message( FATAL_ERROR "UBX_ROOT is not defined. CMake install willl abort." )
endif()

find_package(Ubx REQUIRED)

include_directories(
  ${UBX_INCLUDE_DIR}
  ${CZMQ_INCLUDE_DIRS}
)

# Compile library czmqppworkerlib
add_library(czmqppworkerlib SHARED src/czmq_ppworker.c
modules/czmqppworkerlib_module.c
)
set_target_properties(czmqppworkerlib PROPERTIES PREFIX "")
target_link_libraries(czmqppworkerlib ${UBX_LIBRARIES} ${CZMQ_LIBRARIES})

# Install czmqppworkerlib
install(TARGETS czmqppworkerlib DESTINATION ${INSTALL_LIB_BLOCKS_DIR} EXPORT czmqppworkerlib-block)
set_property(TARGET czmqppworkerlib PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
install(EXPORT czmqppworkerlib-block DESTINATION ${INSTALL_CMAKE_DIR})

