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
