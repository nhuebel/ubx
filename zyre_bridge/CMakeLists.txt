CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(ubx_zyre_bridge)

set(CMAKE_CXX_FLAGS "-Wall -Werror -fvisibility=hidden")
set(CMAKE_CXX_COMPILER clang++ )
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake") ## temporary resources, for UBX

# Global
set(INSTALL_LIB_TYPES_DIR lib/ubx/types CACHE PATH "Installation directory for types libraries")
set(INSTALL_LIB_BLOCKS_DIR lib/ubx/blocks CACHE PATH "Installation directory for blocks libraries")
set(INSTALL_INCLUDE_DIR include/ubx CACHE PATH "Installation directory for header files (types)")
set(INSTALL_CMAKE_DIR  share/ubx/cmake CACHE PATH  "Installation directory for CMake files") # default ${DEF_INSTALL_CMAKE_DIR}

FIND_PACKAGE(ZMQ REQUIRED) 
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
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ubx-zyre-bridge-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/ubx-zyre-bridge-config.cmake" @ONLY
)

set (UBX_ROOT $ENV{UBX_ROOT} )
if (NOT UBX_ROOT)
 message( FATAL_ERROR "UBX_ROOT is not defined. CMake install willl abort." )
endif()

find_package(Microblx REQUIRED)

include_directories(
  ${UBX_INCLUDE_DIR}
  ${ZMQ_INCLUDE_DIRS} 
  ${CZMQ_INCLUDE_DIRS}
)

# Compile library zyresenderlib
add_library(zyresenderlib SHARED src/zyre_sender.cpp )
set_target_properties(zyresenderlib PROPERTIES PREFIX "")
target_link_libraries(zyresenderlib ${UBX_LIBRARIES} ${CZMQ_LIBRARIES})

# Install zyresenderlib
install(TARGETS zyresenderlib DESTINATION ${INSTALL_LIB_BLOCKS_DIR} EXPORT zyresenderlib-block)
set_property(TARGET zyresenderlib PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
install(EXPORT zyresenderlib-block DESTINATION ${INSTALL_CMAKE_DIR})

# Compile library zyrereceiverlib
add_library(zyrereceiverlib SHARED src/zyre_receiver.cpp )
set_target_properties(zyrereceiverlib PROPERTIES PREFIX "")
target_link_libraries(zyrereceiverlib ${UBX_LIBRARIES} ${CZMQ_LIBRARIES})

# Install zyrereceiverlib
install(TARGETS zyrereceiverlib DESTINATION ${INSTALL_LIB_BLOCKS_DIR} EXPORT zyrereceiverlib-block)
set_property(TARGET zyrereceiverlib PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
install(EXPORT zyrereceiverlib-block DESTINATION ${INSTALL_CMAKE_DIR})


