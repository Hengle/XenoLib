project(TEX2DDS)

build_target(
  NAME
  tex_to_dds
  TYPE
  ESMODULE
  VERSION
  1
  SOURCES
  tex_to_dds.cpp
  INCLUDES
  ../common
  LINKS
  xeno
  AUTHOR
  "Lukas Cone"
  DESCR
  "Xenoblade texture to DDS"
  START_YEAR
  2022)
