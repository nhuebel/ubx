CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(cpp_transfer)

set(CMAKE_CXX_FLAGS "-Wall -Werror -fvisibility=hidden")
set(CMAKE_CXX_COMPILER clang++ )
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake") ## temporary resources, for UBX

# Global
set(INSTALL_LIB_TYPES_DIR lib/microblx/types CACHE PATH "Installation directory for types libraries")
set(INSTALL_LIB_BLOCKS_DIR lib/microblx/blocks CACHE PATH "Installation directory for blocks libraries")
set(INSTALL_INCLUDE_DIR include/microblx CACHE PATH "Installation directory for header files (types)")
set(INSTALL_CMAKE_DIR  share/microblx/cmake CACHE PATH  "Installation directory for CMake files") # default ${DEF_INSTALL_CMAKE_DIR}

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
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cpp-transfer-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/cpp-transfer-config.cmake" @ONLY
)

set (UBX_ROOT $ENV{UBX_ROOT} )
if (NOT UBX_ROOT)
 message( FATAL_ERROR "UBX_ROOT is not defined. CMake install willl abort." )
endif()

find_package(Microblx REQUIRED)

include_directories(
  ${UBX_INCLUDE_DIR}
)

set(GEN_HEXARR ${UBX_ROOT}/tools/file2carr.lua)

file(GLOB types
  ${CMAKE_CURRENT_SOURCE_DIR}/types/*.h
)


set(outfiles "")
foreach( _type ${types} )
  string(REPLACE ".h" ".h.hexarr" _outfile ${_type})
  add_custom_command(
  OUTPUT ${_outfile}
  COMMAND ${GEN_HEXARR}
  ARGS ${_type}  > ${_outfile}
  )
  list(APPEND outfiles ${_outfile})
endforeach(_type)
add_custom_target( gen_hexarr ALL DEPENDS ${outfiles} )

add_library(cpp_transfer_types SHARED types/cpp_transfer_types.c)
set_target_properties(cpp_transfer_types PROPERTIES PREFIX "") 
set_property(TARGET cpp_transfer_types PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
target_link_libraries(cpp_transfer_types ${UBX_LIBRARIES})
add_dependencies(cpp_transfer_types gen_hexarr)

### Install libraries
set_target_properties(cpp_transfer_types PROPERTIES PUBLIC_HEADER "${outfiles};${types}" )

install(TARGETS cpp_transfer_types 
  EXPORT cpp_transfer_types-targets
  LIBRARY DESTINATION "${INSTALL_LIB_TYPES_DIR}" COMPONENT cpp_transfer_types
  PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}/types" COMPONENT dev
)

# Compile library cpp_sender
add_library(cpp_sender SHARED ..//cpp_transfer/src/cpp_sender.cpp )
set_target_properties(cpp_sender PROPERTIES PREFIX "")
target_link_libraries(cpp_sender ${UBX_LIBRARIES})
    add_dependencies(cpp_sender gen_hexarr)

# Install cpp_sender
install(TARGETS cpp_sender DESTINATION ${INSTALL_LIB_BLOCKS_DIR} EXPORT cpp_sender-block)
set_property(TARGET cpp_sender PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
install(EXPORT cpp_sender-block DESTINATION ${INSTALL_CMAKE_DIR})

# Compile library cpp_receiver
add_library(cpp_receiver SHARED ..//cpp_transfer/src/cpp_receiver.cpp )
set_target_properties(cpp_receiver PROPERTIES PREFIX "")
target_link_libraries(cpp_receiver ${UBX_LIBRARIES})
    add_dependencies(cpp_receiver gen_hexarr)

# Install cpp_receiver
install(TARGETS cpp_receiver DESTINATION ${INSTALL_LIB_BLOCKS_DIR} EXPORT cpp_receiver-block)
set_property(TARGET cpp_receiver PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
install(EXPORT cpp_receiver-block DESTINATION ${INSTALL_CMAKE_DIR})

