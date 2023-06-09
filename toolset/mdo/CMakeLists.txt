project(MDO2GLTF)

build_target(
  NAME
  mdo_to_gltf
  TYPE
  ESMODULE
  VERSION
  1
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
