project(SMExtract)

build_target(
  NAME
  sm_extract
  TYPE
  ESMODULE
  VERSION
  1
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

project(IM2GLTF)

build_target(
  NAME
  im_to_gltf
  TYPE
  ESMODULE
  VERSION
  1
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

  project(TM2GLTF)

  build_target(
    NAME
    tm_to_gltf
    TYPE
    ESMODULE
    VERSION
    1
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
