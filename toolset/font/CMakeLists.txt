project(FNTExtract)

build_target(
  NAME
  fnt_extract
  TYPE
  ESMODULE
  VERSION
  1
  SOURCES
  fnt_extract.cpp
  INCLUDES
  ../common
  ${TPD_PATH}/spike/3rd_party/json
  LINKS
  xeno
  AUTHOR
  "Lukas Cone"
  DESCR
  "Extract Xenoblade X font to json and texture"
  START_YEAR
  2023)
