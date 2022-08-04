project(TEX2DDS VERSION 1)

build_target(
  NAME
  tex_to_dds
  TYPE
  ESMODULE
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
