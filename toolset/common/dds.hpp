/*  xenoblade_toolset common code
    Copyright(C) 2022 Lukas Cone

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
#include "formats/DDS.hpp"
#include "xenolib/lbim.hpp"
#include "xenolib/mtxt.hpp"

DDS ToDDS(const LBIM::Header *hdr) {
  DDS ddtex = {};
  ddtex = DDSFormat_DX10;
  ddtex.width = hdr->width;
  ddtex.height = hdr->height;
  ddtex.depth = hdr->depth;
  ddtex.NumMipmaps(1);

  if (hdr->type == LBIM::Type::Volumetric) {
    ddtex.flags += DDS::Flags_Depth;
    ddtex.caps01 += DDS::Caps01Flags_Volume;
    ddtex.dimension = DDS::Dimension_3D;
  } else if (hdr->type == LBIM::Type::Cubemap) {
    ddtex.caps01 = decltype(ddtex.caps01)(
        DDS::Caps01Flags_CubeMap, DDS::Caps01Flags_CubeMap_NegativeX,
        DDS::Caps01Flags_CubeMap_NegativeY, DDS::Caps01Flags_CubeMap_NegativeZ,
        DDS::Caps01Flags_CubeMap_PositiveX, DDS::Caps01Flags_CubeMap_PositiveY,
        DDS::Caps01Flags_CubeMap_PositiveZ);
  }

  ddtex.dxgiFormat = [&] {
    switch (hdr->format) {
    case LBIM::Format::BC1:
      return DXGI_FORMAT_BC1_UNORM;
    case LBIM::Format::BC2:
      return DXGI_FORMAT_BC2_UNORM;
    case LBIM::Format::BC3:
      return DXGI_FORMAT_BC3_UNORM;
    case LBIM::Format::BC4_UNORM:
      return DXGI_FORMAT_BC4_UNORM;
    case LBIM::Format::BC5_UNORM:
      return DXGI_FORMAT_BC5_UNORM;
    case LBIM::Format::BC7:
      return DXGI_FORMAT_BC7_UNORM;
    case LBIM::Format::BC6H_UF16:
      return DXGI_FORMAT_BC6H_UF16;
    case LBIM::Format::RGBA8888:
      return DXGI_FORMAT_R8G8B8A8_UNORM;
    case LBIM::Format::RGBA4444:
      return DXGI_FORMAT_B4G4R4A4_UNORM;
    case LBIM::Format::R8:
      return DXGI_FORMAT_R8_UNORM;
    case LBIM::Format::R16G16B16A16:
      return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case LBIM::Format::BGRA8888:
      return DXGI_FORMAT_B8G8R8A8_UNORM;
    default:
      return DXGI_FORMAT_UNKNOWN;
    }
  }();

  ddtex.ComputeBPP();

  return ddtex;
};

DDS ToDDS(const MTXT::Header *hdr) {
  DDS ddtex = {};
  ddtex = DDSFormat_DX10;
  ddtex.width = hdr->width;
  ddtex.height = hdr->height;
  // ddtex.depth = hdr->depth;
  ddtex.NumMipmaps(1);

  ddtex.dxgiFormat = [&] {
    using namespace gx2;
    switch (hdr->type) {
    case gx2::SurfaceFormat::UNORM_BC1:
      return DXGI_FORMAT_BC1_UNORM;
    case gx2::SurfaceFormat::SRGB_BC1:
      return DXGI_FORMAT_BC1_UNORM_SRGB;
    case gx2::SurfaceFormat::UNORM_BC4:
      return DXGI_FORMAT_BC4_UNORM;
    case gx2::SurfaceFormat::SNORM_BC4:
      return DXGI_FORMAT_BC4_SNORM;
    case gx2::SurfaceFormat::UNORM_BC2:
      return DXGI_FORMAT_BC2_UNORM;
    case gx2::SurfaceFormat::SRGB_BC2:
      return DXGI_FORMAT_BC2_UNORM_SRGB;
    case gx2::SurfaceFormat::UNORM_BC3:
      return DXGI_FORMAT_BC3_UNORM;
    case gx2::SurfaceFormat::SRGB_BC3:
      return DXGI_FORMAT_BC3_UNORM_SRGB;
    case gx2::SurfaceFormat::UNORM_BC5:
      return DXGI_FORMAT_BC5_UNORM;
    case gx2::SurfaceFormat::SNORM_BC5:
      return DXGI_FORMAT_BC5_SNORM;
    case gx2::SurfaceFormat::UNORM_R8:
      return DXGI_FORMAT_R8_UNORM;
    case gx2::SurfaceFormat::UNORM_R8_G8_B8_A8:
      return DXGI_FORMAT_R8G8B8A8_UNORM;
    default:
      return DXGI_FORMAT_UNKNOWN;
    }
  }();

  ddtex.ComputeBPP();

  return ddtex;
};
