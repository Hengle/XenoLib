# XenoLib

[![build](https://github.com/PredatorCZ/XenoLib/actions/workflows/cmake.yml/badge.svg)](https://github.com/PredatorCZ/XenoLib/actions/workflows/cmake.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

XenoLib is independent serialize library for various formats used by Xenoblade Engine.\
Library is compilable under Clang 10, Windowns Clang 13.0.1 and G++10.

## Toolset

Toolset can be found in [Toolset folder](https://github.com/PredatorCZ/XenoLib/tree/master/toolset)

[Toolset releases](https://github.com/PredatorCZ/XenoLib/releases)

## Supported formats

* MXMD models (camdo, wimdo)
* DRSM/MXMD streams (casmt, wismt)
* MTXT/LBIM textures (witex, catex)
* XBC1 desompressor
* SAR archives
* BDAT data files
* MTHS shaders
* BC (SKEL, ANIM)

## License

This library is available under GPL v3 license. (See LICENSE.md)

This library uses following libraries:

* Decaf
* PreCore, Copyright (c) 2016-2022 Lukas Cone
* zlib, Copyright (C) 1995-2022 Jean-loup Gailly and Mark Adler
