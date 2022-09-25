project(MDO2GLTF VERSION 1)

build_target(
  NAME
  mdo_to_gltf
  TYPE
  ESMODULE
  SOURCES
  mdo_to_gltf.cpp
  INCLUDES
  ../common
  LINKS
  xeno
  gltf
  AUTHOR
  "Lukas Cone"
  DESCR
  "Xenoblade Model to GLTF"
  START_YEAR
  2022)
