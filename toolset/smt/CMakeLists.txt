project(SMTExtract VERSION 1)

build_target(
  NAME
  smt_extract
  TYPE
  ESMODULE
  SOURCES
  smt_extract.cpp
  INCLUDES
  ../common
  LINKS
  xeno
  AUTHOR
  "Lukas Cone"
  DESCR
  "Extract Textures from WISMT"
  START_YEAR
  2022)
