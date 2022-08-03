project(BDAT2JSON VERSION 1)

build_target(
  NAME
  bdat_to_json
  TYPE
  ESMODULE
  SOURCES
  bdat_to_json.cpp
  INCLUDES
  ${TPD_PATH}/precore/3rd_party/json
  LINKS
  xeno
  AUTHOR
  "Lukas Cone"
  DESCR
  "Convert Xenoblade X Binary Data to JSON"
  START_YEAR
  2022)
