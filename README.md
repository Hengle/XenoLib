# XenoLib

[![build](https://github.com/PredatorCZ/XenoLib/actions/workflows/build.yaml/badge.svg)](https://github.com/PredatorCZ/XenoLib/actions/workflows/build.yaml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

XenoLib is independent serialize library for various formats used by Xenoblade Engine.

Library is compilable under Clang 13+ and GCC 12+.

## Toolset

Toolset can be found in [Toolset folder](https://github.com/PredatorCZ/XenoLib/tree/master/toolset)

[Toolset releases](https://github.com/PredatorCZ/XenoLib/releases)

## Supported formats

* MXMD models (camdo, wimdo)
* DRSM/MXMD streams (casmt, wismt)
* MTXT/LBIM textures (witex, catex)
* XBC1 decompressor
* SAR archives
* BDAT data files
* MTHS shaders
* BC (SKEL, ANIM, ASM)
* ARH/ARD archives
* MSMD streamed maps (casm, wism)

## License

This library is available under GPL v3 license. (See LICENSE.md)

This library uses following libraries:

* Decaf
* Spike, Copyright (c) 2016-2023 Lukas Cone (Apache 2)
* zlib, Copyright (C) 1995-2022 Jean-loup Gailly and Mark Adler (Zlib)
