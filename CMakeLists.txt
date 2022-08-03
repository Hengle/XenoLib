cmake_minimum_required(VERSION 3.12)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "X86 architecture is not supported!")
endif()

project(XenoLib VERSION 1)

# ~~~
# Valid targets:
#  - xeno-static: static library target (XN_STATIC_LIB == ON, force enables havok-objects target)
#  - xeno-objects: static objects target (enabled by default, set NO_OBJECTS to disable all)
#  - xeno: shared library target (BUILD_SHARED_LIBS == ON)
# ~~~

option(TOOLSET "Build toolset." ON)
option(ODR_TEST "Enable ODR testing." OFF)

option(OBJECTS_PID "Imply PID for all objects." ${PYTHON_MODULE})
option(XN_STATIC_LIB "Builds xeno-static target." OFF)

set(CMAKE_CXX_STANDARD 20)
add_compile_options(-Wall -Wextra)

if(TOOLSET)
  set(BUILD_SHARED_LIBS ON)
endif()

set(TPD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party)
add_subdirectory(${TPD_PATH}/precore)
include(targetex)
add_subdirectory(source)

if(TOOLSET)
  add_subdirectory(toolset)
endif()
