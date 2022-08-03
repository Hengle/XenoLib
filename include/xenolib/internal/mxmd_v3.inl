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

namespace V3 {
struct Primitive {
  uint32 ID;
  uint32 skinDescriptor;
  uint16 bufferID;
  uint16 UVFacesID;
  uint16 unk00;
  uint16 materialID;
  uint32 null00[3];
  uint16 unk01;
  uint16 LOD;
  uint32 meshFacesID;
  uint32 null01[3];
};

struct Mesh {
  Array<Primitive> primitives;
  uint32 unk00;
  Vector bboxMax;
  Vector bboxMin;
  float boundingRadius;
};

struct Bone {
  String name;
  float unk00;
  uint32 unk01;
  uint32 ID;
  uint32 null[2];
};

struct Skin {
  uint32 count1;
  uint32 count2;
  Pointer<Bone> nodesOffset;
  Pointer<TransformMatrix> nodeTMSOffset;
  uint32 unkOffset00;
  uint32 unkOffset01;
};

struct MorphControl {
  String name1;
  String name2;
  uint32 data[5];
};

struct Morphs {
  Array<MorphControl> controls;
  uint32 unk[4];
};

struct Model {
  uint32 unk00;
  Vector bboxMin;
  Vector bboxMax;
  Array<Mesh> meshes;
  uint32 null00;
  Pointer<Skin> skin;
  uint32 null01[10];
  uint32 unkCount00;
  uint32 null02[10];
  Pointer<Morphs> morphs;
  uint32 unkOffset00;
  uint32 unkOffset01;
  uint32 unkOffset02;
  uint32 unkOffset03;
  uint32 unkCount01;
};

struct TextureLink : V1::TextureLink {
  uint32 unk01;
};

struct Material {
  Pointer<char> name;
  uint32 unk00;
  float unk01[6];
  Array<TextureLink> textures;
  uint32 unk02[9];
  uint32 linksOffset;
  uint32 linksCount;
  uint32 unk03[8];
};

struct MaterialsHeader {
  Array<Material> materials;
  uint32 unk00[2];
  uint32 floatArrayOffset;
  uint32 floatArrayCount;
  uint32 uintArrayOffset;
  uint32 uintArrayCount;
  uint32 unk01;
  uint32 unkOffset00;
  uint32 unkOffset01;
  uint32 unkCount01;
  uint32 unk02[3];
  uint32 unkOffset02;
  uint32 unk03[3];
  uint32 unkOffset03;
  uint32 unk04[2];
  uint32 unkOffset04;
  uint32 unk05[3];
};

struct SMTHeader : V1::SMTHeader {
  uint32 compressedSize[2];
};

struct VertexBuffer : V1::VertexBuffer {
  uint32 null1[2];
};

struct IndexBuffer : V1::IndexBuffer {
  uint32 null1[2];
};

struct WeightPalette {
  uint32 baseOffset;
  uint32 offsetID;
  uint32 count;
  uint32 unk00[4];
  uint8 unk01;
  uint8 lod;
  uint16 unk02;
  uint32 unk03[2];
};

struct MorphBuffer {
  uint32 vertexBufferOffset;
  uint32 vertexBufferSize;
  uint32 stride;
  uint16 unk;
  uint16 type;
};

struct MorphDescriptor {
  uint32 vertexBufferIndex;
  uint32 bufferIndexBegin;
  uint32 numBuffers;
  Pointer<uint16> targetIds;
  uint32 unk01;
};

struct MorphsHeader {
  Array<MorphDescriptor> descs;
  Array<MorphBuffer> buffers;
};

struct BufferManager {
  uint32 numWeightPalettes;
  Pointer<WeightPalette> weightPalettes;
  uint16 weightBufferID;
  uint16 flags;
  Pointer<MorphsHeader> morph;
};

struct Stream {
  Array<VertexBuffer> vertexBuffers;
  Array<IndexBuffer> indexBuffers;
  uint16 unk00[2];
  uint32 null00[2];
  uint32 unk01;
  uint32 unkOffset00;
  uint32 unkCount00;
  uint32 unk02;
  uint32 bufferSize;
  uint32 bufferOffset;
  uint32 voxelizedModelOffset;
  Pointer<BufferManager> bufferManager;
};

struct Header : HeaderBase {
  Pointer<Model> models;
  Pointer<MaterialsHeader> materials;
  uint32 unk00;
  Pointer<Stream> streams;
  Pointer<char> shaders;
  Pointer<DRSM::Textures> cachedTextures;
  uint32 unk01;
  Pointer<char> uncachedTextures;
  uint32 reserved[7];

  using SMTTextures = std::variant<SMTHeader *, DRSM::Resources *>;

  SMTTextures XN_EXTERN GetSMT();
};
} // namespace V3
