project(ARHExtract VERSION 1)

build_target(
  NAME
  extract_arh
  TYPE
  ESMODULE
  SOURCES
  extract_arh.cpp
  LINKS
  xeno
  AUTHOR
  "Lukas Cone"
  DESCR
  "Extract ARH/ARD archives"
  START_YEAR
  2022)
