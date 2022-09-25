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

namespace MSIM {
using MXMD::Array;
using MXMD::ArrayInv;
using MXMD::Pointer;

namespace V1 {
struct InstancedModel {
  float unk[8];
  int32 modelIndex;
};

struct InstanceLOD {
  uint32 modelIndexStart;
  uint32 numModels;
};

struct InstanceMatrix {
  MXMD::TransformMatrix mtx;
  Vector pos00;
  Vector4 pos01;
  uint32 lodIndex;
  uint32 null00;
  uint32 unk;
  uint32 null01;
};

struct InstancesHeader {
  uint32 null00;
  ArrayInv<InstanceLOD> clusters;
  ArrayInv<InstancedModel> modelLODs;
  ArrayInv<InstanceMatrix> matrices;
  uint32 unk01[16];
  uint32 null01[6];
};

struct Header {
  uint16 unk00;
  uint16 unk01;
  uint32 null02[2];
  Pointer<MXMD::V1::Model> models;
  Pointer<MXMD::V1::MaterialsHeader> materials;
  uint32 unk02;
  Pointer<InstancesHeader> instances;
  uint32 unk03;
  Array<MXMD::V1::ExternalTexture> externalTextures;
  Array<uint32> bufferIndices;
  uint32 unkoffsets01[5];
  Pointer<MXMD::V1::Shaders> shaders;
  Array<uint16> textureContainerIds;
  uint32 unkoffsets02[6];
};
} // namespace V1

} // namespace MSIM
