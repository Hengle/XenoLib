project(SARExtract)

build_target(
  NAME
  sar_extract
  TYPE
  ESMODULE
  VERSION
  1
  SOURCES
  sar_extract.cpp
  LINKS
  xeno-interface
  AUTHOR
  "Lukas Cone"
  DESCR
  "Extract SAR archives"
  START_YEAR
  2022)

project(SARCreate)

build_target(
  NAME
  make_sar
  TYPE
  ESMODULE
  VERSION
  1
  SOURCES
  make_sar.cpp
  LINKS
  xeno-interface
  AUTHOR
  "Lukas Cone"
  DESCR
  "Create SAR archives"
  START_YEAR
  2022)
