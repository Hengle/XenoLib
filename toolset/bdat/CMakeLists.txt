project(BDAT2JSON)

build_target(
  NAME
  bdat_to_json
  TYPE
  ESMODULE
  VERSION
  1
  SOURCES
  bdat_to_json.cpp
  INCLUDES
  ${TPD_PATH}/spike/3rd_party/json
  LINKS
  xeno-interface
  AUTHOR
  "Lukas Cone"
  DESCR
  "Convert Xenoblade X Binary Data to JSON"
  START_YEAR
  2022)
