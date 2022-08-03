project(SARExtract VERSION 1)

build_target(
  NAME
  sar_extract
  TYPE
  ESMODULE
  SOURCES
  sar_extract.cpp
  LINKS
  xeno
  AUTHOR
  "Lukas Cone"
  DESCR
  "Extract SAR archives"
  START_YEAR
  2022)

project(SARCreate VERSION 1)

build_target(
  NAME
  make_sar
  TYPE
  ESMODULE
  SOURCES
  make_sar.cpp
  LINKS
  xeno
  AUTHOR
  "Lukas Cone"
  DESCR
  "Create SAR archives"
  START_YEAR
  2022)
