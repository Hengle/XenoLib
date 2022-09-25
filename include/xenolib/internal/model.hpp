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
#include "uni/list_vector.hpp"
#include "xenolib/mxmd.hpp"
#include <array>
#include <cassert>
#include <optional>

namespace MDO {
using namespace MXMD;
struct PrimitiveDescriptor : uni::PrimitiveDescriptor {
  char *buffer;
  size_t stride;
  size_t offset;
  size_t index;
  uni::PrimitiveDescriptor::Usage_e usage;
  uni::FormatDescr type;
  float unpack = 0.f;
  UnpackDataType_e unpackType = UnpackDataType_e::None;
  const char *RawBuffer() const { return buffer + offset; }
  size_t Stride() const { return stride; }
  size_t Offset() const { return offset; }
  size_t Index() const { return index; }
  Usage_e Usage() const { return usage; }
  uni::FormatDescr Type() const { return type; }
  uni::BBOX UnpackData() const { return {{unpack}, {unpack}}; }
  UnpackDataType_e UnpackDataType() const { return unpackType; }

  operator uni::Element<const uni::PrimitiveDescriptor>() const {
    return uni::Element<const uni::PrimitiveDescriptor>{this, false};
  }
};

struct VertexBuffer : uni::VertexArray {
  uni::VectorList<uni::PrimitiveDescriptor, PrimitiveDescriptor> descs;
  size_t numVertices = 0;

  VertexBuffer() = default;

  VertexBuffer(V1::VertexBuffer &buff) : numVertices(buff.data.numItems) {
    descs.storage.reserve(buff.descriptors.numItems);
    PrimitiveDescriptor desc{};
    desc.buffer = buff.data.items;
    desc.stride = buff.stride;
    size_t curOffset = 0;

    for (auto &d : buff.descriptors) {
      desc.offset = curOffset;
      curOffset += d.size;
      desc.usage = d;
      desc.type = d;
      desc.index = [&] {
        switch (d.type) {
        case VertexDescriptorType::UV2:
        case VertexDescriptorType::VERTEXCOLOR2:
          return 1;
        case VertexDescriptorType::UV3:
        case VertexDescriptorType::VERTEXCOLOR3:
          return 2;
        case VertexDescriptorType::UV4:
          return 3;
        default:
          return 0;
        }
      }();

      descs.storage.emplace_back(desc);
    }
  }

  void AppendMorphBuffer(V3::MorphBuffer *buff, char *buffer) {
    PrimitiveDescriptor desc{};
    desc.buffer = buffer + buff->vertexBufferOffset;
    desc.stride = buff->stride;

    desc.usage = uni::PrimitiveDescriptor::Usage_e::Position;
    desc.type = {uni::FormatType::FLOAT, uni::DataType::R32G32B32};
    descs.storage.emplace_back(desc);

    desc.usage = uni::PrimitiveDescriptor::Usage_e::Normal;
    desc.unpackType = uni::PrimitiveDescriptor::UnpackDataType_e::Add;
    desc.unpack = -0.5f;
    desc.offset = 12;
    desc.type = {uni::FormatType::UNORM, uni::DataType::R8G8B8A8};
    descs.storage.emplace_back(desc);
  }

  uni::PrimitiveDescriptorsConst Descriptors() const override {
    return uni::Element<const uni::List<uni::PrimitiveDescriptor>>(&descs,
                                                                   false);
  }
  size_t NumVertices() const override { return numVertices; }

  operator uni::Element<const uni::VertexArray>() const {
    return uni::Element<const uni::VertexArray>{this, false};
  }
};

struct IndexBuffer : uni::IndexArray {
  uni::VectorList<uni::PrimitiveDescriptor, PrimitiveDescriptor> descs;
  V1::IndexBuffer *buffer;

  IndexBuffer(V1::IndexBuffer &buff) : buffer(&buff) {}

  const char *RawIndexBuffer() const override {
    return reinterpret_cast<const char *>(buffer->indices.items.Get());
  }

  size_t IndexSize() const override { return 2; }
  size_t NumIndices() const override { return buffer->indices.numItems; }

  operator uni::Element<const uni::IndexArray>() const {
    return uni::Element<const uni::IndexArray>{this, false};
  }
};

struct Bone : uni::Bone {
  V1::Bone *bone;
  size_t index;
  Bone *parent = nullptr;

  uni::TransformType TMType() const override {
    return uni::TransformType::TMTYPE_MATRIX;
  }

  void GetTM(es::Matrix44 &out) const override {
    memcpy(&out, &bone->transform, sizeof(bone->transform));
  }
  const Bone *Parent() const override { return parent; }
  size_t Index() const override { return index; }
  std::string Name() const override { return bone->name.Get(); }

  operator uni::Element<const uni::Bone>() const {
    return uni::Element<const uni::Bone>{this, false};
  }
};

struct Skeleton : uni::Skeleton {
  uni::VectorList<uni::Bone, Bone> bones;
  Skeleton() = default;
  Skeleton(V1::Bone *bone, size_t count) {
    bones.storage.resize(count);

    for (size_t i = 0; i < count; i++) {
      Bone bne;
      auto &nBone = bone[i];
      bne.bone = bone + i;
      bne.index = i;

      if (nBone.parentID >= 0) {
        bne.parent = &bones.storage[nBone.parentID];
      }

      bones.storage[i] = bne;
    }
  }

  uni::SkeletonBonesConst Bones() const override {
    return uni::Element<const uni::List<uni::Bone>>(&bones, false);
  }
  std::string Name() const override { return {}; }
};

class Primitive : public uni::Primitive {
public:
  V1::Primitive *main;
  int64 lod = 0;

  Primitive(V1::Primitive &prim) : main(&prim) {}
  std::string Name() const override { return {}; }
  size_t SkinIndex() const override { return 0; }
  int64 LODIndex() const override { return lod; }
  size_t MaterialIndex() const override { return main->materialID; }
  size_t VertexArrayIndex(size_t) const override { return main->bufferID; }
  size_t IndexArrayIndex() const override { return main->UVFacesID; }
  IndexType_e IndexType() const override { return IndexType_e::Triangle; }
  size_t NumVertexArrays() const override { return 1; }

  operator uni::Element<const uni::Primitive>() const {
    return uni::Element<const uni::Primitive>{this, false};
  }
};

class Skin : public uni::Skin {
public:
  V1::MeshSkin *main;
  V1::Bone *bones;

  size_t NumNodes() const override { return main->boneIndices.numItems; }
  uni::TransformType TMType() const override {
    return uni::TransformType::TMTYPE_MATRIX;
  }
  void GetTM(es::Matrix44 &out, size_t index) const override {
    memcpy(&out, &bones[NodeIndex(index)].ibm, sizeof(V1::Bone::ibm));
  }
  size_t NodeIndex(size_t index) const override {
    return main->boneIndices.items[index];
  }

  operator uni::Element<const uni::Skin>() const {
    return uni::Element<const uni::Skin>{this, false};
  }
};

class Material : public uni::Material {
public:
  V1::Material *main;

  size_t Version() const override { return 1; }
  std::string Name() const override { return main->name.Get(); }
  std::string TypeName() const override { return {}; }

  operator uni::Element<const uni::Material>() const {
    return uni::Element<const uni::Material>{this, false};
  }
};

class V1Model : public Model {
public:
  uni::VectorList<uni::Primitive, Primitive> primitives;
  uni::VectorList<uni::Skin, Skin> skins;
  uni::VectorList<uni::Material, Material> materials;
  uni::VectorList<uni::VertexArray, VertexBuffer> vertexArrays;
  uni::VectorList<uni::IndexArray, IndexBuffer> indexArrays;

  V1Model() = default;
  V1Model(V1::Header &main);

  uni::PrimitivesConst Primitives() const override {
    return uni::Element<const uni::List<uni::Primitive>>(&primitives, false);
  }
  uni::SkinsConst Skins() const override {
    return uni::Element<const uni::List<uni::Skin>>(&skins, false);
  }
  uni::IndexArraysConst Indices() const override {
    return uni::Element<const uni::List<uni::IndexArray>>(&indexArrays, false);
  }
  uni::VertexArraysConst Vertices() const override {
    return uni::Element<const uni::List<uni::VertexArray>>(&vertexArrays,
                                                           false);
  }
  uni::ResourcesConst Resources() const override { return {}; }
  uni::MaterialsConst Materials() const override {
    return uni::Element<const uni::List<uni::Material>>(&materials, false);
  }

  const Morphs *MorphTargets() const override { return nullptr; }
  const WeightSamplers_t *WeightSamplers() const override { return nullptr; }
  const MorphNames MorphTargetNames() const override { return {}; }
};

struct V3Bone : uni::Bone {
  V3::Bone *bone;
  size_t index;

  uni::TransformType TMType() const override {
    return uni::TransformType::TMTYPE_RTS;
  }

  /*void GetTM(uni::RTSValue &out) const override {
    //memcpy(&out., &bone->transform, sizeof(bone->transform));
  }*/

  const Bone *Parent() const override { return nullptr; }
  size_t Index() const override { return index; }
  std::string Name() const override { return bone->name.Get(); }

  operator uni::Element<const uni::Bone>() const {
    return uni::Element<const uni::Bone>{this, false};
  }
};

struct V3Skeleton : uni::Skeleton {
  uni::VectorList<uni::Bone, V3Bone> bones;
  V3Skeleton() = default;
  V3Skeleton(V3::Skin *skin) {
    bones.storage.resize(skin->count1);

    for (size_t i = 0; i < skin->count1; i++) {
      V3Bone bne;
      bne.bone = skin->nodes.Get() + i;
      bne.index = i;
      bones.storage[i] = bne;
    }
  }

  uni::SkeletonBonesConst Bones() const override {
    return uni::Element<const uni::List<uni::Bone>>(&bones, false);
  }
  std::string Name() const override { return {}; }
};

class V3Skin : public uni::Skin {
public:
  V3::Skin *main;

  size_t NumNodes() const override { return main->count1; }
  uni::TransformType TMType() const override {
    return uni::TransformType::TMTYPE_MATRIX;
  }
  void GetTM(es::Matrix44 &out, size_t index) const override {
    memcpy(&out, main->nodeIBMs.Get() + index, sizeof(TransformMatrix));
  }
  size_t NodeIndex(size_t index) const override { return index; }

  operator uni::Element<const uni::Skin>() const {
    return uni::Element<const uni::Skin>{this, false};
  }
};

class V3Primitive : public uni::Primitive {
public:
  V3::Primitive *main;

  V3Primitive(V3::Primitive &prim) : main(&prim) {}
  std::string Name() const override { return {}; }
  size_t SkinIndex() const override { return 0; }
  int64 LODIndex() const override { return main->LOD; }
  size_t MaterialIndex() const override { return main->materialID; }
  size_t VertexArrayIndex(size_t) const override { return main->bufferID; }
  size_t IndexArrayIndex() const override { return main->meshFacesID; }
  IndexType_e IndexType() const override { return IndexType_e::Triangle; }
  size_t NumVertexArrays() const override { return 1; }

  operator uni::Element<const uni::Primitive>() const {
    return uni::Element<const uni::Primitive>{this, false};
  }
};

class V3Morph : public Morph {
public:
  uni::VectorList<uni::PrimitiveDescriptor, PrimitiveDescriptor> descs;
  size_t numVertices = 0;
  size_t targetBuffer = 0;
  size_t index;

  V3Morph() = default;
  V3Morph(V3::MorphBuffer *buff, char *buffer)
      : numVertices(buff->vertexBufferSize) {
    PrimitiveDescriptor desc{};
    desc.buffer = buffer + buff->vertexBufferOffset;
    desc.stride = buff->stride;

    desc.usage = uni::PrimitiveDescriptor::Usage_e::PositionDelta;
    desc.type = {uni::FormatType::FLOAT, uni::DataType::R32G32B32};
    descs.storage.emplace_back(desc);

    desc.usage = uni::PrimitiveDescriptor::Usage_e::VertexIndex;
    desc.type = {uni::FormatType::UINT, uni::DataType::R32};
    desc.offset = 28;
    descs.storage.emplace_back(desc);

    desc.usage = uni::PrimitiveDescriptor::Usage_e::Normal;
    desc.unpackType = uni::PrimitiveDescriptor::UnpackDataType_e::Add;
    desc.unpack = -0.5f;
    desc.offset = 16;
    desc.type = {uni::FormatType::UNORM, uni::DataType::R8G8B8A8};
    descs.storage.emplace_back(desc);
  }

  size_t Index() const override { return index; }
  size_t TargetVertexArrayIndex() const override { return targetBuffer; }
  uni::PrimitiveDescriptorsConst Descriptors() const override {
    return uni::Element<const uni::List<uni::PrimitiveDescriptor>>(&descs,
                                                                   false);
  }
  size_t NumVertices() const override { return numVertices; }

  operator uni::Element<const Morph>() const {
    return uni::Element<const Morph>{this, false};
  }
};

class V3WeightSampler : public WeightSampler {
public:
  uint32 bufferOffset = 0;
  VertexBuffer *buffer = nullptr;

  V3WeightSampler() = default;
  V3WeightSampler(V3::WeightPalette &pal, VertexBuffer &buff)
      : bufferOffset{pal.vertexIndexSubstract - pal.bufferVertexBegin},
        buffer{&buff} {}

  void Resample(const std::vector<uint32> &indices,
                std::vector<uint32> &outBoneIds,
                std::vector<uint32> &outBoneWeights) const override {
    outBoneWeights.resize(indices.size());
    outBoneIds.resize(indices.size());

    for (auto &d : buffer->descs.storage) {
      switch (d.Usage()) {
      case PrimitiveDescriptor::Usage_e::BoneWeights: {
        auto &codec = d.Codec();
        for (size_t idx = 0; auto i : indices) {
          size_t index = i + bufferOffset;
          Vector4A16 v;
          codec.GetValue(v, d.RawBuffer() + index * d.Stride());
          v.w = std::max(1.f - v.x - v.y - v.z, 0.f);
          v *= 0xff;
          v = Vector4A16(_mm_round_ps(v._data, _MM_ROUND_NEAREST));
          auto comp = v.Convert<uint8>();
          outBoneWeights.at(idx++) = reinterpret_cast<uint32 &>(comp);
        }

        break;
      }
      case PrimitiveDescriptor::Usage_e::BoneIndices: {
        auto &codec = d.Codec();
        for (size_t idx = 0; auto i : indices) {
          size_t index = i + bufferOffset;
          IVector4A16 v;
          codec.GetValue(v, d.RawBuffer() + index * d.Stride());
          auto comp = v.Convert<uint8>();
          outBoneIds.at(idx++) = reinterpret_cast<uint32 &>(comp);
        }

        break;
      }
      default:
        throw std::runtime_error("Unhandled vertex weight type");
      }
    }
  }

  operator uni::Element<const WeightSampler>() const {
    return uni::Element<const WeightSampler>{this, false};
  }
};

class V3WeightSamplers_t : public WeightSamplers_t {
public:
  std::vector<std::array<V3WeightSampler, 16>> samplers;
  V3::SkinManager *man;

  V3WeightSamplers_t() = default;
  V3WeightSamplers_t(V3::SkinManager *man_, VertexBuffer &buff) : man(man_) {
    samplers.resize(man->numLODs);

    for (auto &p : man->weightPalettes) {
      std::construct_at(
          &samplers.at(p.mergeTableIndex).at(p.indexWithinMergeTable), p, buff);
    }
  }

  const WeightSampler *Get(const uni::Primitive *prim) const override {
    auto v3Prim = static_cast<const V3Primitive *>(prim);
    auto sFlags = v3Prim->main->skinFlags;
    uint32 palIndex = 0;

    while (sFlags && (sFlags & 1) == 0) {
      palIndex++;
      sFlags >>= 1;
    }

    auto lod = std::max(v3Prim->LODIndex() - 1, int64(0));
    auto &lodSmpl = samplers.at(lod);

    auto smplIndex = &lodSmpl.at(palIndex);
    if (!smplIndex->buffer) {
      for (auto &s : lodSmpl) {
        if (s.buffer) {
          smplIndex = &s;
          break;
        }
      }
    }

    assert(smplIndex->buffer != nullptr);

    return smplIndex;
  }
};

class V3Model : public Model {
public:
  uni::VectorList<uni::Primitive, V3Primitive> primitives;
  uni::VectorList<uni::Skin, V3Skin> skins;
  uni::VectorList<uni::VertexArray, VertexBuffer> vertexArrays;
  uni::VectorList<uni::IndexArray, IndexBuffer> indexArrays;
  uni::VectorList<Morph, V3Morph> morphs;
  std::optional<V3WeightSamplers_t> weightSamplers;
  std::vector<es::string_view> morphNames;
  std::string streamBuffer;
  V3::Stream *stream = nullptr;

  V3Model() = default;
  V3Model(V3::Model *model, BinReaderRef rd);

  uni::PrimitivesConst Primitives() const override {
    return uni::Element<const uni::List<uni::Primitive>>(&primitives, false);
  }
  uni::IndexArraysConst Indices() const override {
    return uni::Element<const uni::List<uni::IndexArray>>(&indexArrays, false);
  }
  uni::VertexArraysConst Vertices() const override {
    return uni::Element<const uni::List<uni::VertexArray>>(&vertexArrays,
                                                           false);
  }
  uni::SkinsConst Skins() const override {
    return uni::Element<const uni::List<uni::Skin>>(&skins, false);
  }
  uni::ResourcesConst Resources() const override { return {}; }
  uni::MaterialsConst Materials() const override { return {}; }
  const Morphs *MorphTargets() const override { return &morphs; }
  const WeightSamplers_t *WeightSamplers() const override {
    return weightSamplers ? &weightSamplers.value() : nullptr;
  }
  const MorphNames MorphTargetNames() const override { return morphNames; }
};
} // namespace MDO
