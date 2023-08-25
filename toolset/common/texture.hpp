/*  xenoblade_toolset common code
    Copyright(C) 2022-2023 Lukas Cone

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
#include "spike/app_context.hpp"
#include "spike/util/endian.hpp"
#include "xenolib/lbim.hpp"
#include "xenolib/mtxt.hpp"
#include <optional>

NewTexelContextCreate MakeContext(const LBIM::Header *hdr, const void *data) {
  NewTexelContextCreate nctx{
      .width = uint16(hdr->width),
      .height = uint16(hdr->height),
      .baseFormat =
          {
              .type =
                  [&] {
                    using T = TexelInputFormatType;
                    switch (hdr->format) {
                    case LBIM::Format::BC1:
                      return T::BC1;
                    case LBIM::Format::BC2:
                      return T::BC2;
                    case LBIM::Format::BC3:
                      return T::BC3;
                    case LBIM::Format::BC4_UNORM:
                      return T::BC4;
                    case LBIM::Format::BC5_UNORM:
                      return T::BC5;
                    case LBIM::Format::BC7:
                      return T::BC7;
                    case LBIM::Format::BC6H_UF16:
                      return T::BC6;
                    case LBIM::Format::RGBA8888:
                    case LBIM::Format::BGRA8888:
                      return T::RGBA8;
                    case LBIM::Format::RGBA4444:
                      return T::RGBA4;
                    case LBIM::Format::R8:
                      return T::R8;
                    case LBIM::Format::R16G16B16A16:
                      return T::RGBA16;
                    default:
                      return T::INVALID;
                    }
                  }(),
              .tile = TexelTile::NX,
          },
      .depth = uint16(hdr->depth),
      .numFaces = int8(hdr->type == LBIM::Type::Cubemap ? 6 : 0),
      .data = data,
  };

  if (hdr->format == LBIM::Format::BGRA8888) {
    nctx.baseFormat.swizzle[0] = TexelSwizzle::Blue;
    nctx.baseFormat.swizzle[2] = TexelSwizzle::Red;
  }

  return nctx;
}

struct MTXTAddrLib : TileBase {
  uint32 width;
  uint32 height;
  uint32 BPT;
  const MTXT::Header *hdr;
  std::optional<MTXT::AddrLibMacroTilePrecomp> precompThick;
  std::optional<MTXT::AddrLibMicroTilePrecomp> precompThin;

  MTXTAddrLib() = default;

  MTXTAddrLib(const MTXT::Header *hdr_, uint32 BPT_) : BPT(BPT_), hdr(hdr_) {}

  void reset(uint32 width_, uint32 height_, uint32) {
    width = width_;
    height = height_;
    const uint32 pipeSwizzle = (hdr->swizzle >> 8) & 1;
    const uint32 bankSwizzle = (hdr->swizzle >> 9) & 3;

    if (hdr->tilingMode >= gx2::TileMode::Tiled2DThin1) {
      precompThick = MTXT::AddrLibMacroTilePrecomp(BPT * 8, hdr->pitch, height,
                                                   hdr->tilingMode, pipeSwizzle,
                                                   bankSwizzle);
    } else {
      precompThin =
          MTXT::AddrLibMicroTilePrecomp(BPT * 8, hdr->pitch, hdr->tilingMode);
    }
  }

  uint32 get(uint32 inTexel) const override {
    const uint32 x = inTexel % width;
    const uint32 y = inTexel / width;

    if (precompThick) {
      return MTXT::computeSurfaceAddrFromCoordMacroTiled(x, y, *precompThick) /
             BPT;
    }

    return computeSurfaceAddrFromCoordMicroTiled(x, y, *precompThin) / BPT;
  }
};

NewTexelContextCreate MakeContext(const MTXT::Header *hdr, MTXTAddrLib &addrlib,
                                  const void *data) {
  uint32 BPT = 8;

  NewTexelContextCreate nctx{
      .width = uint16(hdr->width),
      .height = uint16(hdr->height),
      .baseFormat = {
          .type =
              [&] {
                using T = TexelInputFormatType;
                switch (hdr->type) {
                case gx2::SurfaceFormat::UNORM_BC1:
                case gx2::SurfaceFormat::SRGB_BC1:
                  return T::BC1;
                case gx2::SurfaceFormat::UNORM_BC4:
                case gx2::SurfaceFormat::SNORM_BC4:
                  return T::BC4;
                case gx2::SurfaceFormat::UNORM_BC2:
                case gx2::SurfaceFormat::SRGB_BC2:
                  BPT = 16;
                  return T::BC2;
                case gx2::SurfaceFormat::UNORM_BC3:
                case gx2::SurfaceFormat::SRGB_BC3:
                  BPT = 16;
                  return T::BC3;
                case gx2::SurfaceFormat::UNORM_BC5:
                case gx2::SurfaceFormat::SNORM_BC5:
                  BPT = 16;
                  return T::BC5;
                case gx2::SurfaceFormat::UNORM_R8:
                  BPT = 1;
                  return T::R8;
                case gx2::SurfaceFormat::UNORM_R8_G8_B8_A8:
                  BPT = 4;
                  return T::RGBA8;
                default:
                  return T::INVALID;
                }
              }(),
          .tile = hdr->tilingMode > gx2::TileMode::LinearAligned
                      ? TexelTile::Custom
                      : TexelTile::Linear,
          .srgb =
              [&] {
                switch (hdr->type) {
                case gx2::SurfaceFormat::SRGB_BC1:
                case gx2::SurfaceFormat::SNORM_BC4:
                case gx2::SurfaceFormat::SRGB_BC2:
                case gx2::SurfaceFormat::SRGB_BC3:
                case gx2::SurfaceFormat::SNORM_BC5:
                  return true;
                default:
                  return false;
                }
              }(),
      },
      .numMipmaps = uint8(hdr->numMips),
      .data = data,
  };

  if (hdr->tilingMode > gx2::TileMode::LinearAligned) {
    addrlib = MTXTAddrLib(hdr, BPT);
    nctx.customTile = &addrlib;
  }

  return nctx;
}

void SendTextureLB(std::string_view texData, AppExtractContext *ectx,
                   std::string name) {
  auto tex = LBIM::Mount(texData);
  ectx->NewImage(name, MakeContext(tex, texData.data()));
}

void SendTextureGTX(std::string_view texData, AppExtractContext *ectx,
                    std::string name) {
  auto tex = MTXT::Mount(texData);
  MTXTAddrLib arrdLib;
  FByteswapper(*const_cast<MTXT::Header *>(tex));
  ectx->NewImage(name, MakeContext(tex, arrdLib, texData.data()));
}
