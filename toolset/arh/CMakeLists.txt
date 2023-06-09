project(ARHExtract)

build_target(
  NAME
  extract_arh
  TYPE
  ESMODULE
  VERSION
  1
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
