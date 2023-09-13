project(SMTExtract)

build_target(
  NAME
  smt_extract
  TYPE
  ESMODULE
  VERSION
  1
  SOURCES
  smt_extract.cpp
  INCLUDES
  ../common
  LINKS
  xeno-interface
  AUTHOR
  "Lukas Cone"
  DESCR
  "Extract Textures from WISMT"
  START_YEAR
  2022)
