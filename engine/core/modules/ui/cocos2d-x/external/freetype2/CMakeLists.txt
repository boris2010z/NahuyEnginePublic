cmake_minimum_required(VERSION 3.6)

set(lib_name freetype)
set(target_name ext_${lib_name})

project(${lib_name} LANGUAGES C)

include(../cmake/CocosExternalConfig.cmake)

add_library(${target_name} STATIC IMPORTED GLOBAL)

set_target_properties(${target_name} PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include/${platform_name}/freetype2"
)

if(WINDOWS)
  set_target_properties(${target_name} PROPERTIES
    IMPORTED_LOCATION "${platform_spec_path}/${lib_name}.lib")
  install(FILES "${platform_spec_path}/${lib_name}.lib" 
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    RENAME ${target_name}.lib)
else()
  set_target_properties(${target_name} PROPERTIES
    IMPORTED_LOCATION "${platform_spec_path}/lib${lib_name}.a")
  install(FILES "${platform_spec_path}/lib${lib_name}.a"
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    RENAME ${target_name}.a)
endif()


