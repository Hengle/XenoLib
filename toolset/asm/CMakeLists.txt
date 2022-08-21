project(ASM2JSON VERSION 1)

build_target(
  NAME
  asm_to_json
  TYPE
  ESMODULE
  SOURCES
  asm_to_json.cpp
  INCLUDES
  ${TPD_PATH}/precore/3rd_party/json
  LINKS
  xeno
  AUTHOR
  "Lukas Cone"
  DESCR
  "Convert Xenoblade Animation Assembly to JSON"
  START_YEAR
  2022)
