﻿# CMakeList.txt : CMake project for project_tool, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

if(BUILD_SHARED_LIBS)
  add_subdirectory(cmd)
  add_subdirectory(tool)
  if (NAU_CORE_TESTS)
    add_subdirectory(tests)
  endif()
endif()

# TODO: Add tests and install targets if needed.
