project(DecBDAT)

build_target(
  NAME
  decrypt_bdat
  TYPE
  ESMODULE
  VERSION
  1
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

project(DecARH)

build_target(
  NAME
  decrypt_arh
  TYPE
  ESMODULE
  VERSION
  1
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
