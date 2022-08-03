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

namespace V1 {
struct Primitive {
  uint32 flags;
  uint32 skinDescriptor;
  uint32 bufferID;
  uint32 UVFacesID;
  uint32 unk00;
  uint32 materialID;
  uint32 null00[2];
  uint32 gibID;
  uint32 null01;
  uint32 meshFacesID;
  uint32 null02[5];
};

struct Mesh {
  Array<Primitive> primitives;
  uint32 null0;
  Vector bboxMax;
  Vector bboxMin;
  float boundingRadius;
  uint32 null[7];
};

struct MeshSkin {
  Array<uint16> boneIndices;

  float *GetFloats() {
    return reinterpret_cast<float *>(boneIndices.end() +
                                     (boneIndices.numItems & 1));
  }
};

struct Bone {
  String name;
  int32 parentID;
  int32 firstChild;
  int32 lastChild;
  int32 unk00;
  Vector position;
  Vector rotation; // euler radian
  Vector scale;
  TransformMatrix ibm;
  TransformMatrix transform;
};

struct Model {
  Vector bboxMin;
  Vector bboxMax;
  Array<Mesh> meshes;
  Array<MeshSkin> skins;
  uint32 unk00[3];

  int attachmentsOffset2, unk01[6]; // TODO
  Array<Bone> bones;
  Array<float> floats;
  uint32 unk02;
  Array<String> boneNames;
  uint32 unk03, attachmentsOffset, attachmentsCount, unkOffset00, unk04[4],
      unkOffset01, unk05; // TODO
};

struct VertexBuffer {
  Array<char> data;
  uint32 stride;
  Array<VertexType> descriptors;
  uint32 null;
};

struct IndexBuffer {
  Array<uint16> indices;
  uint32 null; // index type??
};

struct Stream {
  Array<VertexBuffer> vertexBuffers;
  Array<IndexBuffer> indexBuffers;
  int16 mergeData[16];
};

struct SMTHeader {
  uint32 unk; // group start?
  uint32 numUsedGroups;
  Pointer<DRSM::Textures> groups[2];
  Pointer<int16> groupIds[2];
  uint32 dataOffsets[2];
  uint32 dataSizes[2];
};

struct TextureLink {
  int16 textureID;
  int16 unk; // sampler slot?
};

struct Material {
  String name;
  uint32 unk00;
  float unk01[13];
  Array<TextureLink> textures;
  uint32 unk02[9];
  uint32 linksOffset;
  uint32 linksCount;
  uint32 null[6];
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

struct Shader {
  Array<char> data;
  uint32 null[2];
};

struct Shaders {
  Array<Shader> shaders;
  uint32 unk;
  uint32 null[5];
};

struct ExternalTexture {
  uint16 textureID;
  uint16 containerID;
  uint16 externalTextureID;
  uint16 unk;
};

struct Header : HeaderBase {
  Pointer<Model> models;
  Pointer<MaterialsHeader> materials;
  uint32 unk00;
  Pointer<Stream> streams;
  Pointer<Shaders> shaders;
  Pointer<DRSM::Textures> cachedTextures;
  uint32 unk01;
  Pointer<SMTHeader> uncachedTextures;
  uint32 reserved[7];
};

} // namespace V1
