project(DecBDAT VERSION 1)

build_target(
  NAME
  decrypt_bdat
  TYPE
  ESMODULE
  SOURCES
  decrypt_bdat.cpp
  LINKS
  xeno
  AUTHOR
  "Lukas Cone"
  DESCR
  "Decrypt Xenoblade X Binary Data for analysis"
  START_YEAR
  2022)

project(DecARH VERSION 1)

build_target(
  NAME
  decrypt_arh
  TYPE
  ESMODULE
  SOURCES
  decrypt_arh.cpp
  LINKS
  xeno
  AUTHOR
  "Lukas Cone"
  DESCR
  "Decrypt Xenoblade ARH for analysis"
  START_YEAR
  2022)
