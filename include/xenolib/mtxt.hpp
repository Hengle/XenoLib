/*  Xenoblade Engine Format Library
    Copyright(C) 2017-2023 Lukas Cone

    This program is free software : you can redistribute it and / or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#include "core.hpp"
#include "internal/gx2.hpp"
#include "spike/util/supercore.hpp"
#include <string_view>

namespace MTXT {
static constexpr uint32 ID = CompileFourCC("MTXT");

struct Header {
  uint32 swizzle;
  gx2::SurfaceDim dimension;
  uint32 width;
  uint32 height;
  uint32 depth;
  uint32 numMips;
  gx2::SurfaceFormat type;
  uint32 size;
  gx2::AAMode AAMode;
  gx2::TileMode tilingMode;
  uint32 unk;
  uint32 alignment;
  uint32 pitch;
  uint32 null[13];
  uint32 version;
  uint32 id;
};

const Header *Mount(std::string_view data) {
  return reinterpret_cast<const Header *>(&*data.end() - sizeof(Header));
}

struct AddrLibMacroTilePrecomp {
  uint32 microTileThickness, microTileBits, microTileBytes, numSamples,
      swizzle_, sliceBytes, macroTileAspectRatio, macroTilePitch,
      macroTileHeight, macroTilesPerRow, macroTileBytes, bankSwapWidth,
      swappedBank, bpp;
  mutable uint32 sampleSlice;

  XN_EXTERN AddrLibMacroTilePrecomp(uint32 _bpp, uint32 pitch, uint32 height,
                                    gx2::TileMode tileMode, uint32 pipeSwizzle,
                                    uint32 bankSwizzle);
};

struct AddrLibMicroTilePrecomp {
  uint32 microTileBytes, microTilesPerRow;
  uint32 bpp;

  XN_EXTERN AddrLibMicroTilePrecomp(uint32 _bpp, uint32 pitch,
                                    gx2::TileMode tileMode);
};

uint32 XN_EXTERN computeSurfaceAddrFromCoordMacroTiled(
    uint32 x, uint32 y, const AddrLibMacroTilePrecomp &precomp);

uint32 XN_EXTERN computeSurfaceAddrFromCoordMicroTiled(
    uint32 x, uint32 y, const AddrLibMicroTilePrecomp &precomp);

} // namespace MTXT
