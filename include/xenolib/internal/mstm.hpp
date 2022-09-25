/*  Xenoblade Engine Format Library
    Copyright(C) 2017-2022 Lukas Cone

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

#include "mxmd.hpp"

namespace MSTM {
using MXMD::Array;
using MXMD::Pointer;

namespace V1 {
struct TerrainBufferLookup {
  float bbox[6];
  uint32 bufferIndex[2];
  uint32 null;
};

struct TerrainBufferLookupHeader {
  Array<TerrainBufferLookup> bufferLookups;
  Array<uint16> meshIndices;
  uint32 unk00;
  uint32 null00[6];
};

struct Header {
  uint16 unk00;
  uint16 unk01;
  uint32 null02[2];
  Pointer<MXMD::V1::Model> models;
  Pointer<MXMD::V1::MaterialsHeader> materials;
  uint32 unk02;
  uint32 unk03;
  Array<MXMD::V1::ExternalTexture> externalTextures;
  uint32 unkoffsets01[2];
  Pointer<MXMD::V1::Shaders> shaders;
  Array<uint16> textureContainerIds;
  Pointer<TerrainBufferLookupHeader> buffers;
  uint32 unkoffsets02[7];
};
} // namespace V1

} // namespace MSTM
