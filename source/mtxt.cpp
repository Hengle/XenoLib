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

#include "xenolib/mtxt.hpp"
#include "addrlib.inl"
#include "spike/util/endian.hpp"
#include <cassert>
#include <string>

template <> void XN_EXTERN FByteswapper(MTXT::Header &item, bool) {
  FArraySwapper(item);
}

namespace MTXT {
void DecodeMipmap(const MTXT::Header &header, const char *data, char *outData,
                  uint32 mipIndex) {
  using namespace gx2;
  assert(mipIndex == 0); // not implemented
  uint32 bpp = 0;        // bytes per block/pixel
  uint32 ppb = 1;        // pixels per block

  switch (header.type) {
  case gx2::SurfaceFormat::UNORM_BC1:
  case gx2::SurfaceFormat::SRGB_BC1:
  case gx2::SurfaceFormat::UNORM_BC4:
  case gx2::SurfaceFormat::SNORM_BC4:
    bpp = 8;
    ppb = 4;
    break;
  case gx2::SurfaceFormat::SRGB_BC2:
  case gx2::SurfaceFormat::UNORM_BC2:
  case gx2::SurfaceFormat::SRGB_BC3:
  case gx2::SurfaceFormat::UNORM_BC3:
  case gx2::SurfaceFormat::SNORM_BC5:
  case gx2::SurfaceFormat::UNORM_BC5:
    bpp = 16;
    ppb = 4;
    break;
  case gx2::SurfaceFormat::UNORM_R8:
    bpp = 1;
    break;
  case gx2::SurfaceFormat::UNORM_R8_G8_B8_A8:
    bpp = 4;
    break;
  default:
    throw std::runtime_error("Unhandled MTXT texture format: " +
                             std::to_string(header.type));
  }

  const uint32 width = header.width / ppb;
  const uint32 height = header.height / ppb;
  const uint32 surfaceSize = width * height * bpp;

  if (header.tilingMode > gx2::TileMode::LinearAligned) {
    const uint32 pipeSwizzle = (header.swizzle >> 8) & 1;
    const uint32 bankSwizzle = (header.swizzle >> 9) & 3;

    if (header.tilingMode >= gx2::TileMode::Tiled2DThin1) {
      const AddrLibMacroTilePrecomp precomp(bpp * 8, header.pitch, height,
                                            header.tilingMode, pipeSwizzle,
                                            bankSwizzle);

      for (uint32 w = 0; w < width; w++) {
        for (uint32 h = 0; h < height; h++) {
          uint32 address = computeSurfaceAddrFromCoordMacroTiled(w, h, precomp);
          memcpy(outData + ((h * width + w) * bpp), data + address, bpp);
        }
      }
    } else {
      const AddrLibMicroTilePrecomp precomp(bpp * 8, header.pitch,
                                            header.tilingMode);

      for (uint32 w = 0; w < width; w++) {
        for (uint32 h = 0; h < height; h++) {
          uint32 address = computeSurfaceAddrFromCoordMicroTiled(w, h, precomp);
          memcpy(outData + ((h * width + w) * bpp), data + address, bpp);
        }
      }
    }
  } else {
    memcpy(outData, data, surfaceSize);
  }
}
} // namespace MTXT
