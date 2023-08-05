project(ASM2JSON)

build_target(
  NAME
  asm_to_json
  TYPE
  ESMODULE
  VERSION
  1
  SOURCES
  asm_to_json.cpp
  INCLUDES
  ${TPD_PATH}/spike/3rd_party/json
  LINKS
  xeno
  AUTHOR
  "Lukas Cone"
  DESCR
  "Convert Xenoblade Animation Assembly to JSON"
  START_YEAR
  2022)
