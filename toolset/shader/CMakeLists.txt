project(SHDExtract VERSION 1)

add_subdirectory(${TPD_PATH}/decaf ${CMAKE_CURRENT_BINARY_DIR}/decaf)

build_target(
  NAME
  extract_shaders
  TYPE
  ESMODULE
  SOURCES
  extract_shaders.cpp
  LINKS
  xeno
  decaf
  AUTHOR
  "Lukas Cone"
  DESCR
  "Extract and disassemble shaders from anything"
  START_YEAR
  2022)
