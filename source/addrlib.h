/*      AddrLib is a part of GTX Extractor, ported and optimized into C
        languange.
   
        Copyright(C) 2015-2018 AboodXD
        Copyright(C) 2018-2019 Lukas Cone

        This program is free software : you can redistribute it and /or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        (at your option) any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program.If not, see < http://www.gnu.org/licenses/>.
*/

#pragma once

struct AddrLibMacroTilePrecomp {
  unsigned int microTileThickness, microTileBits, microTileBytes, numSamples,
      swizzle_, sliceBytes, macroTileAspectRatio, macroTilePitch,
      macroTileHeight, macroTilesPerRow, macroTileBytes, bankSwapWidth,
      swappedBank, bpp;
  mutable unsigned int sampleSlice;

  AddrLibMacroTilePrecomp(unsigned int _bpp, unsigned int pitch,
                          unsigned int height, gx2::TileMode tileMode,
                          unsigned int pipeSwizzle, unsigned int bankSwizzle);
};

struct AddrLibMicroTilePrecomp {
  unsigned int microTileBytes, microTilesPerRow;
  unsigned int bpp;

  AddrLibMicroTilePrecomp(unsigned int _bpp, unsigned int pitch,
                          gx2::TileMode tileMode);
};

unsigned int
computeSurfaceAddrFromCoordMacroTiled(unsigned int x, unsigned int y,
                                      const AddrLibMacroTilePrecomp &precomp);

unsigned int
computeSurfaceAddrFromCoordMicroTiled(unsigned int x, unsigned int y,
                                      const AddrLibMicroTilePrecomp &precomp);
