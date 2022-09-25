project(SMExtract VERSION 1)

build_target(
  NAME
  sm_extract
  TYPE
  ESMODULE
  SOURCES
  sm_extract.cpp
  INCLUDES
  ../common
  LINKS
  xeno
  AUTHOR
  "Lukas Cone"
  DESCR
  "Extract Streamed Map from Xenoblade"
  START_YEAR
  2022)

project(IM2GLTF VERSION 1)

build_target(
  NAME
  im_to_gltf
  TYPE
  ESMODULE
  SOURCES
  im_to_gltf.cpp
  INCLUDES
  ../common
  LINKS
  xeno
  gltf
  AUTHOR
  "Lukas Cone"
  DESCR
  "Xenoblade Instanced Model to GLTF"
  START_YEAR
  2022)

  project(TM2GLTF VERSION 1)

  build_target(
    NAME
    tm_to_gltf
    TYPE
    ESMODULE
    SOURCES
    tm_to_gltf.cpp
    INCLUDES
    ../common
    LINKS
    xeno
    gltf
    AUTHOR
    "Lukas Cone"
    DESCR
    "Xenoblade Terrain Model to GLTF"
    START_YEAR
    2022)
