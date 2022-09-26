# XenobladeToolset

Xenoblade Toolset is a collection of conversion tools under XenoLib.

This toolset runs on Spike foundation.

Head to this **[Wiki](https://github.com/PredatorCZ/PreCore/wiki/Spike)** for more information on how to effectively use it.

## ASM2JSON

### Module command: asm_to_json

Converts Animation State Machine graph to JSON.

## BDAT2JSON

### Module command: bdat_to_json

Converts BDAT data file or BDAT collection to JSON.

### Settings

- **extract**

  **CLI Long:** ***--extract***\
  **CLI Short:** ***-E***

  **Default value:** true

  Extract by data entry if possible.

## ARHExtract

### Module command: extract_arh

Extract files from ARH/ARD archive pair.

## SHDExtract

### Module command: extract_shaders

Extract shaders from models, shader bundles or shaders. Converts them into GLSL code and/or shader assembly.

## IM2GLTF

### Module command: im_to_gltf

Convert streamed map instanced meshes to GLTF.

## SARCreate

### Module command: make_sar

Create SAR archives from files.

### Settings

- **extension**

  **CLI Long:** ***--extension***\
  **CLI Short:** ***-e***

  **Default value:** sar

  Set output file extension. (common: sar, chr, mot)

- **big-endian**

  **CLI Long:** ***--big-endian***\
  **CLI Short:** ***-e***

  **Default value:** false

  Output platform is big endian.

## MDO2GLTF

### Module command: mdo_to_gltf

Convert models to GLTF.

### Settings

- **fallback-skeleton**

  **CLI Long:** ***--fallback-skeleton***\
  **CLI Short:** ***-f***

  Fallback skeleton file name if none wasn't found.

## SARExtract

### Module command: sar_extract

Exctract files from SAR archives.

## SMExtract

### Module command: sm_extract

Extract contents of streamed maps.

## SMTExtract

### Module command: smt_extract

Extract and convert textures from stream files into DDS.

## TEX2DDS

### Module command: tex_to_dds

Convert textures or streamed textures into DDS format.

## TM2GLTF

### Module command: tm_to_gltf

Convert streamed map terrain model to GLTF.
