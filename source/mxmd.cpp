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

#include "xenolib/mxmd.hpp"
#include "datas/binreader_stream.hpp"
#include "datas/endian.hpp"
#include "datas/except.hpp"
#include "uni/list_vector.hpp"
#include "xenolib/internal/mxmd.hpp"

namespace MXMD {

VertexType::operator uni::FormatDescr() const {
  switch (type) {
  case VertexDescriptorType::POSITION:
  case VertexDescriptorType::NORMAL32:
  case VertexDescriptorType::WEIGHT32:
    return {uni::FormatType::FLOAT, uni::DataType::R32G32B32};
  case VertexDescriptorType::NORMAL:
  case VertexDescriptorType::NORMAL2:
    return {uni::FormatType::NORM, uni::DataType::R8G8B8A8};
  case VertexDescriptorType::BONEID:
  case VertexDescriptorType::BONEID2:
    return {uni::FormatType::UINT, uni::DataType::R8G8B8A8};
  case VertexDescriptorType::WEIGHTID:
    return {uni::FormatType::UINT, uni::DataType::R16};
  case VertexDescriptorType::VERTEXCOLOR:
    return {uni::FormatType::UINT, uni::DataType::R8G8B8A8};
  case VertexDescriptorType::UV1:
  case VertexDescriptorType::UV2:
  case VertexDescriptorType::UV3:
    return {uni::FormatType::FLOAT, uni::DataType::R32G32};
  case VertexDescriptorType::WEIGHT16:
    return {uni::FormatType::UNORM, uni::DataType::R16G16B16A16};
  case VertexDescriptorType::MORPHVERTEXID:
    return {uni::FormatType::UINT, uni::DataType::R32};
  case VertexDescriptorType::NORMALMORPH:
    return {uni::FormatType::UNORM, uni::DataType::R8G8B8A8};
  default:
    throw std::runtime_error("Unhandled vertex type");
  }
}

VertexType::operator uni::PrimitiveDescriptor::Usage_e() const {
  using ut = uni::PrimitiveDescriptor::Usage_e;
  switch (type) {
  case VertexDescriptorType::POSITION:
    return ut::Position;
  case VertexDescriptorType::NORMAL32:
  case VertexDescriptorType::NORMAL:
  case VertexDescriptorType::NORMAL2:
  case VertexDescriptorType::NORMALMORPH:
    return ut::Normal;
  case VertexDescriptorType::WEIGHT32:
  case VertexDescriptorType::WEIGHT16:
    return ut::BoneWeights;
  case VertexDescriptorType::BONEID:
  case VertexDescriptorType::BONEID2:
    return ut::BoneIndices;
  case VertexDescriptorType::WEIGHTID:
  case VertexDescriptorType::MORPHVERTEXID:
    return ut::VertexIndex;
  case VertexDescriptorType::VERTEXCOLOR:
    return ut::VertexColor;
  case VertexDescriptorType::UV1:
  case VertexDescriptorType::UV2:
  case VertexDescriptorType::UV3:
    return ut::TextureCoordiante;
  default:
    throw std::runtime_error("Unhandled vertex type");
  }
}

namespace {
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
} // namespace

struct VertexBuffer {
  uni::VectorList<uni::PrimitiveDescriptor, PrimitiveDescriptor> descs;

  VertexBuffer(V1::VertexBuffer &buff) {
    descs.storage.reserve(buff.descriptors.numItems);
    PrimitiveDescriptor desc;
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
        case VertexDescriptorType::MORPHVERTEXID:
          return 1;
        case VertexDescriptorType::UV3:
          return 2;
        case VertexDescriptorType::NORMALMORPH:
          desc.unpackType = uni::PrimitiveDescriptor::UnpackDataType_e::Add;
          desc.unpack = -0.5f;
          return 0;
        default:
          return 0;
        }
      }();

      descs.storage.emplace_back(desc);
    }
  }
};
} // namespace MXMD

using namespace MXMD;

template <class C> void FByteswapper(Array<C> &item, bool = false) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(MXMD::HeaderBase &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(V1::MeshSkin &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(V1::Bone &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(V1::Primitive &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(V1::Mesh &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(V1::Model &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(V1::TextureLink &item, bool) {
  FArraySwapper<uint16>(item);
}

template <> void XN_EXTERN FByteswapper(V1::Material &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(V1::MaterialsHeader &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(MXMD::VertexType &item, bool) {
  FArraySwapper<uint16>(item);
}

template <> void XN_EXTERN FByteswapper(V1::VertexBuffer &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(V1::IndexBuffer &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(V1::Stream &item, bool) {
  FByteswapper(item.vertexBuffers);
  FByteswapper(item.indexBuffers);
  FByteswapper(item.mergeData);
}

template <> void XN_EXTERN FByteswapper(V1::SMTHeader &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(V1::ExternalTexture &item, bool) {
  FArraySwapper<uint16>(item);
}

template <> void XN_EXTERN FByteswapper(V1::Shader &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(V1::Shaders &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(V1::Header &item, bool) {
  FArraySwapper(item);
}

template <>
void XN_EXTERN ProcessClass(V1::MeshSkin &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoAutoDetect();
  flags.NoLittleEndian();
  FByteswapper(item);
  item.boneIndices.Fixup(flags.base);

  for (auto &p : item.boneIndices) {
    FByteswapper(p);
  }
}

template <> void XN_EXTERN ProcessClass(V1::Bone &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoAutoDetect();
  flags.NoLittleEndian();
  FByteswapper(item);
  item.name.Fixup(flags.base);
}

template <> void XN_EXTERN ProcessClass(V1::Mesh &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoAutoDetect();
  flags.NoLittleEndian();
  FByteswapper(item);
  item.primitives.Fixup(flags.base);

  for (auto &p : item.primitives) {
    FByteswapper(p);
  }
}

template <> void XN_EXTERN ProcessClass(V1::Model &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoAutoDetect();
  flags.NoLittleEndian();
  FByteswapper(item);
  flags.base = reinterpret_cast<char *>(&item);
  es::FixupPointers(flags.base, item.boneNames, item.bones, item.floats,
                    item.meshes, item.skins);

  for (auto &b : item.boneNames) {
    FByteswapper(b);
    b.Fixup(flags.base);
  }

  auto boneNamesFlags = flags;
  boneNamesFlags.base = reinterpret_cast<char *>(item.bones.begin());

  for (auto &b : item.bones) {
    ProcessClass(b, boneNamesFlags);
  }

  for (auto &f : item.floats) {
    FByteswapper(f);
  }

  for (auto &m : item.meshes) {
    ProcessClass(m, flags);
  }

  for (auto &s : item.skins) {
    ProcessClass(s, flags);
  }
}

template <>
void XN_EXTERN ProcessClass(V1::Material &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoAutoDetect();
  flags.NoLittleEndian();
  FByteswapper(item);
  item.textures.Fixup(flags.base);

  for (auto &t : item.textures) {
    FByteswapper(t);
  }
}

template <>
void XN_EXTERN ProcessClass(V1::MaterialsHeader &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoAutoDetect();
  flags.NoLittleEndian();
  FByteswapper(item);
  flags.base = reinterpret_cast<char *>(&item);
  item.materials.Fixup(flags.base);

  for (auto &m : item.materials) {
    ProcessClass(m, flags);
  }
}

template <>
void XN_EXTERN ProcessClass(V1::VertexBuffer &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoAutoDetect();

  if (flags == ProcessFlag::EnsureBigEndian) {
    FByteswapper(item);
  }

  item.data.Fixup(flags.base);
  item.descriptors.Fixup(flags.base);

  if (flags != ProcessFlag::EnsureBigEndian) {
    return;
  }
  for (auto &d : item.descriptors) {
    FByteswapper(d);
  }

  const size_t numVerts = item.data.numItems / item.stride;
  char *buffer = item.data.items.Get();

  for (auto &d : item.descriptors) {
    switch (d.type) {
    case VertexDescriptorType::POSITION:
    case VertexDescriptorType::NORMAL32:
    case VertexDescriptorType::WEIGHT32:
      for (size_t i = 0; i < numVerts; i++) {
        char *bufBegin = buffer + item.stride * i;
        FByteswapper(*reinterpret_cast<Vector *>(bufBegin));
      }
      break;
    case VertexDescriptorType::WEIGHTID:
      for (size_t i = 0; i < numVerts; i++) {
        char *bufBegin = buffer + item.stride * i;
        FByteswapper(*reinterpret_cast<uint16 *>(bufBegin));
      }
      break;
    case VertexDescriptorType::UV1:
    case VertexDescriptorType::UV2:
    case VertexDescriptorType::UV3:
      for (size_t i = 0; i < numVerts; i++) {
        char *bufBegin = buffer + item.stride * i;
        FByteswapper(*reinterpret_cast<Vector2 *>(bufBegin));
      }
      break;
    case VertexDescriptorType::WEIGHT16:
    case VertexDescriptorType::TANGENT16:
      for (size_t i = 0; i < numVerts; i++) {
        char *bufBegin = buffer + item.stride * i;
        FByteswapper(*reinterpret_cast<SVector4 *>(bufBegin));
      }
      break;
    case VertexDescriptorType::MORPHVERTEXID:
      for (size_t i = 0; i < numVerts; i++) {
        char *bufBegin = buffer + item.stride * i;
        FByteswapper(*reinterpret_cast<uint32 *>(bufBegin));
      }
      break;
    case VertexDescriptorType::NORMAL:
    case VertexDescriptorType::NORMAL2:
    case VertexDescriptorType::TANGENT:
    case VertexDescriptorType::BONEID:
    case VertexDescriptorType::BONEID2:
    case VertexDescriptorType::VERTEXCOLOR:
    case VertexDescriptorType::NORMALMORPH:
    case VertexDescriptorType::REFLECTION:
      break;
    default:
      throw std::runtime_error("Unhandled vertex type");
    }

    buffer += d.size;
  }
}

template <>
void XN_EXTERN ProcessClass(V1::IndexBuffer &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoAutoDetect();

  if (flags == ProcessFlag::EnsureBigEndian) {
    FByteswapper(item);
  }
  item.indices.Fixup(flags.base);

  if (flags == ProcessFlag::EnsureBigEndian) {
    for (auto &i : item.indices) {
      FByteswapper(i);
    }
  }
}

template <> void XN_EXTERN ProcessClass(V1::Stream &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoAutoDetect();
  flags.NoLittleEndian();
  FByteswapper(item);
  flags.base = reinterpret_cast<char *>(&item);
  es::FixupPointers(flags.base, item.vertexBuffers, item.indexBuffers);

  for (auto &v : item.vertexBuffers) {
    ProcessClass(v, flags);
  }

  for (auto &i : item.indexBuffers) {
    ProcessClass(i, flags);
  }
}

template <>
void XN_EXTERN ProcessClass(V1::SMTHeader &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoAutoDetect();

  if (flags == ProcessFlag::EnsureBigEndian) {
    FArraySwapper(item);
  }

  if (item.unk > 1) [[unlikely]] {
    throw std::runtime_error("SMTHeader.unk should be <2");
  }

  if (item.numUsedGroups > 2 || !item.numUsedGroups) [[unlikely]] {
    throw std::runtime_error("SMTHeader.numGroups should be <= 2");
  }

  flags.base = reinterpret_cast<char *>(&item);

  for (uint32 i = 0; i < item.numUsedGroups; i++) {
    es::FixupPointers(flags.base, item.groupIds[i], item.groups[i]);
    DRSM::Textures *group = item.groups[i];
    ProcessClass(*group, flags);
    if (flags == ProcessFlag::EnsureBigEndian) {
      int16 *ids = item.groupIds[i];

      for (uint32 t = 0; t < group->textures.numItems; t++) {
        FByteswapper(ids[t]);
      }
    }
  }
}

template <> void XN_EXTERN ProcessClass(V1::Shader &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoAutoDetect();
  flags.NoLittleEndian();
  FByteswapper(item);
  item.data.Fixup(flags.base);
}

template <> void XN_EXTERN ProcessClass(V1::Shaders &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoAutoDetect();
  flags.NoLittleEndian();
  FByteswapper(item);
  flags.base = reinterpret_cast<char *>(&item);
  item.shaders.Fixup(flags.base);

  for (auto &s : item.shaders) {
    ProcessClass(s, flags);
  }
}

template <>
void XN_EXTERN ProcessClass(V3::SMTHeader &item, ProcessFlags flags) {
  flags.NoBigEndian();
  ProcessClass<V1::SMTHeader>(item, flags);
}

template <>
void XN_EXTERN ProcessClass(V3::VertexBuffer &item, ProcessFlags flags) {
  flags.NoBigEndian();
  ProcessClass<V1::VertexBuffer>(item, flags);
}

template <>
void XN_EXTERN ProcessClass(V3::IndexBuffer &item, ProcessFlags flags) {
  flags.NoBigEndian();
  ProcessClass<V1::IndexBuffer>(item, flags);
}

template <>
void XN_EXTERN ProcessClass(V3::MorphDescriptor &item, ProcessFlags flags) {
  flags.NoBigEndian();
  flags.NoProcessDataOut();
  flags.NoAutoDetect();

  item.targetIds.Fixup(flags.base);
}

template <>
void XN_EXTERN ProcessClass(V3::MorphsHeader &item, ProcessFlags flags) {
  flags.NoBigEndian();
  flags.NoProcessDataOut();
  flags.NoAutoDetect();

  es::FixupPointers(flags.base, item.descs, item.buffers);
}

template <>
void XN_EXTERN ProcessClass(V3::BufferManager &item, ProcessFlags flags) {
  flags.NoBigEndian();
  flags.NoProcessDataOut();
  flags.NoAutoDetect();

  es::FixupPointers(flags.base, item.weightPalettes, item.morph);

  if (item.morph) {
    // todo item.morph correct offset
    // ProcessClass(*item.morph, flags);
  }
}

template <> void XN_EXTERN ProcessClass(V3::Stream &item, ProcessFlags flags) {
  flags.NoBigEndian();
  flags.NoProcessDataOut();
  flags.NoAutoDetect();
  flags.base = reinterpret_cast<char *>(&item);

  es::FixupPointers(flags.base, item.vertexBuffers, item.indexBuffers,
                    item.bufferManager);

  for (auto &v : item.vertexBuffers) {
    ProcessClass(v, flags);
  }

  for (auto &i : item.indexBuffers) {
    ProcessClass(i, flags);
  }

  if (item.bufferManager) {
    ProcessClass(*item.bufferManager, flags);
  }
}

template <> void XN_EXTERN ProcessClass(V3::Mesh &item, ProcessFlags flags) {
  flags.NoBigEndian();
  flags.NoProcessDataOut();
  flags.NoAutoDetect();
  item.primitives.Fixup(flags.base);
}

template <> void XN_EXTERN ProcessClass(V3::Bone &item, ProcessFlags flags) {
  flags.NoBigEndian();
  flags.NoProcessDataOut();
  flags.NoAutoDetect();
  item.name.Fixup(flags.base);
}

template <> void XN_EXTERN ProcessClass(V3::Skin &item, ProcessFlags flags) {
  flags.NoBigEndian();
  flags.NoProcessDataOut();
  flags.NoAutoDetect();
  es::FixupPointers(flags.base, item.nodesOffset, item.nodeTMSOffset);
}

template <>
void XN_EXTERN ProcessClass(V3::MorphControl &item, ProcessFlags flags) {
  flags.NoBigEndian();
  flags.NoProcessDataOut();
  flags.NoAutoDetect();
  es::FixupPointers(flags.base, item.name1, item.name2);
}

template <> void XN_EXTERN ProcessClass(V3::Morphs &item, ProcessFlags flags) {
  flags.NoBigEndian();
  flags.NoProcessDataOut();
  flags.NoAutoDetect();
  item.controls.Fixup(flags.base);

  for (auto &c : item.controls) {
    ProcessClass(c, flags);
  }
}

template <> void XN_EXTERN ProcessClass(V3::Model &item, ProcessFlags flags) {
  flags.NoBigEndian();
  flags.NoProcessDataOut();
  flags.NoAutoDetect();
  es::FixupPointers(flags.base, item.meshes, item.morphs, item.skin);

  for (auto &c : item.meshes) {
    ProcessClass(c, flags);
  }

  if (item.morphs) {
    ProcessClass(*item.morphs, flags);
  }

  if (item.skin) {
    ProcessClass(*item.skin, flags);
  }
}

template <>
void XN_EXTERN ProcessClass(V3::Material &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoAutoDetect();
  flags.NoBigEndian();
  item.textures.Fixup(flags.base);
}

template <>
void XN_EXTERN ProcessClass(V3::MaterialsHeader &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoAutoDetect();
  flags.NoBigEndian();
  flags.base = reinterpret_cast<char *>(&item);
  item.materials.Fixup(flags.base);

  for (auto &m : item.materials) {
    ProcessClass(m, flags);
  }
}

template <> void XN_EXTERN ProcessClass(V1::Header &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoAutoDetect();
  flags.NoLittleEndian();
  FByteswapper(item);
  flags.base = reinterpret_cast<char *>(&item);
  Wrap::ExcludeLoads exLoads = static_cast<Wrap::ExcludeLoads>(flags.userData);
  es::FixupPointers(flags.base, item.models, item.materials, item.streams,
                    item.shaders, item.cachedTextures, item.uncachedTextures);

  if (exLoads != Wrap::ExcludeLoad::Model && item.models) {
    ProcessClass(*item.models, flags);
    ProcessClass(*item.streams, flags);
  } else {
    item.models.Reset();
    item.streams.Reset();
  }

  if (exLoads != Wrap::ExcludeLoad::Shaders && item.shaders) {
    ProcessClass(*item.shaders, flags);
  } else {
    item.shaders.Reset();
  }

  if (exLoads != Wrap::ExcludeLoad::Materials && item.materials) {
    ProcessClass(*item.materials, flags);
  } else {
    item.materials.Reset();
  }

  if (exLoads != Wrap::ExcludeLoad::LowTextures && item.cachedTextures) {
    ProcessClass(*item.cachedTextures, flags);
  } else {
    item.cachedTextures.Reset();
  }

  if (exLoads != Wrap::ExcludeLoad::TextureStreams && item.uncachedTextures) {
    ProcessClass(*item.uncachedTextures, flags);
  } else {
    item.uncachedTextures.Reset();
  }
}

template <> void XN_EXTERN ProcessClass(V2::Header &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoAutoDetect();
  flags.NoBigEndian();
  flags.base = reinterpret_cast<char *>(&item);
  Wrap::ExcludeLoads exLoads = static_cast<Wrap::ExcludeLoads>(flags.userData);
  es::FixupPointers(flags.base, item.models, item.materials, item.streams,
                    item.shaders, item.cachedTextures, item.uncachedTextures);

  /*if (exLoads != Wrap::ExcludeLoad::Model && item.models) {
    ProcessClass(*item.models, flags);
    ProcessClass(*item.streams, flags);
  } else {
    item.models.Reset();
    item.streams.Reset();
  }

  if (exLoads != Wrap::ExcludeLoad::Shaders && item.shaders) {
    ProcessClass(*item.shaders, flags);
  } else {
    item.shaders.Reset();
  }

  if (exLoads != Wrap::ExcludeLoad::Materials && item.materials) {
    ProcessClass(*item.materials, flags);
  } else {
    item.materials.Reset();
  }
  */
  if (exLoads != Wrap::ExcludeLoad::LowTextures && item.cachedTextures) {
    ProcessClass(*item.cachedTextures, flags);
  } else {
    item.cachedTextures.Reset();
  }

  if (exLoads != Wrap::ExcludeLoad::TextureStreams && item.uncachedTextures) {
    ProcessClass(*item.uncachedTextures, flags);
  } else {
    item.uncachedTextures.Reset();
  }
}

template <> void XN_EXTERN ProcessClass(V3::Header &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoAutoDetect();
  flags.NoBigEndian();
  flags.base = reinterpret_cast<char *>(&item);
  Wrap::ExcludeLoads exLoads = static_cast<Wrap::ExcludeLoads>(flags.userData);
  es::FixupPointers(flags.base, item.models, item.materials, item.streams,
                    item.shaders, item.cachedTextures, item.uncachedTextures);

  if (exLoads != Wrap::ExcludeLoad::Model && item.models) {
    ProcessClass(*item.models, flags);
    if (item.streams) {
      ProcessClass(*item.streams, flags);
    }
  } else {
    item.models.Reset();
    item.streams.Reset();
  }

  if (exLoads != Wrap::ExcludeLoad::Shaders && item.shaders) {
    // ProcessClass(*item.shaders, flags);
  } else {
    item.shaders.Reset();
  }

  if (exLoads != Wrap::ExcludeLoad::Materials && item.materials) {
    ProcessClass(*item.materials, flags);
  } else {
    item.materials.Reset();
  }

  if (exLoads != Wrap::ExcludeLoad::LowTextures && item.cachedTextures) {
    ProcessClass(*item.cachedTextures, flags);
  } else {
    item.cachedTextures.Reset();
  }

  if (exLoads != Wrap::ExcludeLoad::TextureStreams && item.uncachedTextures) {
    std::visit([flags](auto item) { ProcessClass(*item, flags); },
               item.GetSMT());
  } else {
    item.uncachedTextures.Reset();
  }
}

template <>
void XN_EXTERN ProcessClass(MXMD::HeaderBase &item, ProcessFlags flags) {
  flags.NoProcessDataOut();

  if (flags == ProcessFlag::AutoDetectEndian) {
    flags -= ProcessFlag::AutoDetectEndian;

    if (item.magic == ID_BIG) {
      flags += ProcessFlag::EnsureBigEndian;
      FByteswapper(item.version);
    } else if (item.magic == ID) {
      flags -= ProcessFlag::EnsureBigEndian;
    } else {
      throw es::InvalidHeaderError(item.magic);
    }
  }

  if (item.version == MXMD::Versions::MXMDVer1) {
    FByteswapper(item.version);
    ProcessClass(static_cast<V1::Header &>(item), flags);
  } else if (item.version == MXMD::Versions::MXMDVer2) {
    ProcessClass(static_cast<V2::Header &>(item), flags);
  } else if (item.version == MXMD::Versions::MXMDVer3) {
    ProcessClass(static_cast<V3::Header &>(item), flags);
  } else {
    throw es::InvalidVersionError(item.version);
  }
}

V3::Header::SMTTextures V3::Header::GetSMT() {
  if (cachedTextures) {
    return reinterpret_cast<V3::SMTHeader *>(uncachedTextures.Get());
  }

  return reinterpret_cast<DRSM::Resources *>(uncachedTextures.Get());
}

namespace {
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

class Primitive : public uni::Primitive, public VertexBuffer {
public:
  using VertexBuffer::VertexBuffer;
  V1::Primitive *main;
  V1::IndexBuffer *indexBuffer;
  V1::VertexBuffer *vertexBuffer;

  const char *RawIndexBuffer() const override {
    return reinterpret_cast<const char *>(indexBuffer->indices.items.Get());
  }
  const char *RawVertexBuffer(size_t) const override {
    return vertexBuffer->data.items.Get();
  }
  uni::PrimitiveDescriptorsConst Descriptors() const override {
    return uni::Element<const uni::List<uni::PrimitiveDescriptor>>(&descs,
                                                                   false);
  }
  IndexType_e IndexType() const override { return IndexType_e::Triangle; }
  size_t IndexSize() const override { return 2; }
  size_t NumVertices() const override {
    return vertexBuffer->data.numItems / vertexBuffer->stride;
  }
  size_t NumVertexBuffers() const override { return 1; }
  size_t NumIndices() const override { return indexBuffer->indices.numItems; }
  std::string Name() const override { return {}; }
  size_t SkinIndex() const override { return 0; }
  size_t LODIndex() const override { return 0; }
  size_t MaterialIndex() const override { return main->materialID; }

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

class Model : public uni::Model {
public:
  uni::VectorList<uni::Primitive, Primitive> primitives;
  uni::VectorList<uni::Skin, Skin> skins;
  uni::VectorList<uni::Material, Material> materials;

  Model() = default;
  Model(V1::Header &main) {
    auto model = main.models.Get();
    auto streams = main.streams.Get();
    auto mats = main.materials.Get();

    for (auto &m : model->meshes) {
      for (auto &p : m.primitives) {
        auto vtx = streams->vertexBuffers.items.Get() + p.bufferID;
        Primitive prim(*vtx);
        prim.main = &p;
        prim.vertexBuffer = vtx;
        prim.indexBuffer = streams->indexBuffers.items.Get() + p.meshFacesID;
        primitives.storage.emplace_back(prim);
      }
    }

    for (auto &s : model->skins) {
      Skin sk;
      sk.main = &s;
      sk.bones = model->bones.items.Get();
      skins.storage.emplace_back(sk);
    }

    for (auto &m : mats->materials) {
      Material mat;
      mat.main = &m;
      materials.storage.emplace_back(mat);
    }
  }

  uni::PrimitivesConst Primitives() const override {
    return uni::Element<const uni::List<uni::Primitive>>(&primitives, false);
  }
  uni::SkinsConst Skins() const override {
    return uni::Element<const uni::List<uni::Skin>>(&skins, false);
  }
  uni::ResourcesConst Resources() const override { return {}; }
  uni::MaterialsConst Materials() const override {
    return uni::Element<const uni::List<uni::Material>>(&materials, false);
  }
};
} // namespace

namespace MXMD {

class Impl {
public:
  std::string buffer;
  BinReaderRef stream;
  Skeleton skel;
  Model model;

  void Load(BinReaderRef rd, BinReaderRef stream_,
            Wrap::ExcludeLoads excludeLoads) {
    stream = stream_;
    rd.ReadContainer(buffer, rd.GetSize());

    HeaderBase &hdr = reinterpret_cast<HeaderBase &>(*buffer.data());
    ProcessFlags flags{ProcessFlag::AutoDetectEndian};
    flags.userData = static_cast<uint32>(excludeLoads);
    ProcessClass(hdr, flags);

    if (hdr.version == Versions::MXMDVer1) {
      V1::Header &main = static_cast<V1::Header &>(hdr);
      if (main.models) {
        skel = Skeleton(main.models->bones.items, main.models->bones.numItems);
        model = Model(main);
      }
    }
  }

  Variant GetVariant() {
    HeaderBase &hdr = reinterpret_cast<HeaderBase &>(*buffer.data());

    if (hdr.version == Versions::MXMDVer1) {
      return std::ref(static_cast<V1::Header &>(hdr));
    } else if (hdr.version == Versions::MXMDVer2) {
      return std::ref(static_cast<V2::Header &>(hdr));
    } else if (hdr.version == Versions::MXMDVer3) {
      return std::ref(static_cast<V3::Header &>(hdr));
    }

    throw std::runtime_error("Invalid MXMD version");
  }
};

Wrap::Wrap() : pi(std::make_unique<Impl>()) {}
Wrap::Wrap(Wrap &&) = default;
Wrap::~Wrap() = default;

void Wrap::Load(BinReaderRef main, BinReaderRef stream,
                ExcludeLoads excludeLoads) {
  pi->Load(main, stream, excludeLoads);
}

Wrap::operator const uni::Skeleton *() { return &pi->skel; }
Wrap::operator const uni::Model *() { return &pi->model; }

class WrapFriend : public Wrap {
public:
  using Wrap::pi;
};

Variant GetVariantFromWrapper(Wrap &wp) {
  return static_cast<WrapFriend &>(wp).pi->GetVariant();
}
} // namespace MXMD
