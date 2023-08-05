cmake_minimum_required(VERSION 3.12)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "X86 architecture is not supported!")
endif()

project(XenoLib VERSION 1)

# ~~~
# Valid targets:
#  - xeno-static: static library target (XN_STATIC_LIB == ON, force enables xeno-objects target)
#  - xeno-objects: static objects target (enabled by default, set NO_OBJECTS to disable all)
#  - xeno: shared library target (BUILD_SHARED_LIBS == ON)
# ~~~

option(TOOLSET "Build toolset." ON)
option(ODR_TEST "Enable ODR testing." OFF)

option(OBJECTS_PID "Imply PID for all objects." OFF)
option(XN_STATIC_LIB "Builds xeno-static target." OFF)

option(NO_OBJECTS "" OFF)
option(CLI "" ${TOOLSET})
option(GLTF "" ${TOOLSET})
option(BUILD_SHARED_LIBS "" ${TOOLSET})

set(CMAKE_CXX_STANDARD 20)
add_compile_options(-Wall -Wextra)

set(TPD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party)
add_subdirectory(${TPD_PATH}/spike)
include(targetex)
add_subdirectory(source)

if(TOOLSET)
  add_subdirectory(toolset)
endif()
