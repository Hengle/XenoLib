project(SHDExtract)

add_subdirectory(${TPD_PATH}/decaf ${CMAKE_CURRENT_BINARY_DIR}/decaf)

build_target(
  NAME
  extract_shaders
  TYPE
  ESMODULE
  VERSION
  1
  SOURCES
  extract_shaders.cpp
  LINKS
  xeno-interface
  decaf
  AUTHOR
  "Lukas Cone"
  DESCR
  "Extract and disassemble shaders from anything"
  START_YEAR
  2022)
