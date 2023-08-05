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

#include "xenolib/mxmd.hpp"
#include "spike/except.hpp"
#include "spike/io/binreader_stream.hpp"
#include "spike/uni/list_vector.hpp"
#include "spike/util/endian.hpp"
#include "xenolib/drsm.hpp"
#include "xenolib/internal/model.hpp"

namespace MXMD {

VertexType::operator uni::FormatDescr() const {
  switch (type) {
  case VertexDescriptorType::POSITION:
  case VertexDescriptorType::NORMAL32:
  case VertexDescriptorType::WEIGHT32:
    return {uni::FormatType::FLOAT, uni::DataType::R32G32B32};
  case VertexDescriptorType::NORMAL:
  case VertexDescriptorType::TANGENT:
  case VertexDescriptorType::TANGENT2:
  case VertexDescriptorType::NORMAL2:
    return {uni::FormatType::NORM, uni::DataType::R8G8B8A8};
  case VertexDescriptorType::TANGENT16:
    return {uni::FormatType::NORM, uni::DataType::R16G16B16A16};
  case VertexDescriptorType::BONEID:
  case VertexDescriptorType::BONEID2:
    return {uni::FormatType::UINT, uni::DataType::R8G8B8A8};
  case VertexDescriptorType::WEIGHTID:
    return {uni::FormatType::UINT, uni::DataType::R16};
  case VertexDescriptorType::VERTEXCOLOR:
  case VertexDescriptorType::VERTEXCOLOR2:
  case VertexDescriptorType::VERTEXCOLOR3:
    return {uni::FormatType::UINT, uni::DataType::R8G8B8A8};
  case VertexDescriptorType::UV1:
  case VertexDescriptorType::UV2:
  case VertexDescriptorType::UV3:
  case VertexDescriptorType::UV4:
    return {uni::FormatType::FLOAT, uni::DataType::R32G32};
  case VertexDescriptorType::WEIGHT16:
    return {uni::FormatType::UNORM, uni::DataType::R16G16B16A16};
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
    return ut::Normal;
  case VertexDescriptorType::WEIGHT32:
  case VertexDescriptorType::WEIGHT16:
    return ut::BoneWeights;
  case VertexDescriptorType::BONEID:
  case VertexDescriptorType::BONEID2:
    return ut::BoneIndices;
  case VertexDescriptorType::WEIGHTID:
    return ut::VertexIndex;
  case VertexDescriptorType::VERTEXCOLOR:
  case VertexDescriptorType::VERTEXCOLOR2:
  case VertexDescriptorType::VERTEXCOLOR3:
    return ut::VertexColor;
  case VertexDescriptorType::UV1:
  case VertexDescriptorType::UV2:
  case VertexDescriptorType::UV3:
  case VertexDescriptorType::UV4:
    return ut::TextureCoordiante;
  case VertexDescriptorType::TANGENT:
  case VertexDescriptorType::TANGENT2:
  case VertexDescriptorType::TANGENT16:
    return ut::Tangent;
  default:
    throw std::runtime_error("Unhandled vertex type");
  }
}
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

  if (item.boneNames.items) {
    for (auto &b : item.boneNames) {
      FByteswapper(b);
      b.Fixup(flags.base);
    }

    auto boneNamesFlags = flags;
    boneNamesFlags.base = reinterpret_cast<char *>(item.bones.begin());

    for (auto &b : item.bones) {
      ProcessClass(b, boneNamesFlags);
    }
  }

  /*for (auto &f : item.floats) {
    FByteswapper(f);
  }*/

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
  flags.NoLittleEndian();

  FByteswapper(item);

  item.data.Fixup(flags.base);
  item.descriptors.Fixup(flags.base);

  for (auto &d : item.descriptors) {
    FByteswapper(d);
  }

  const size_t numVerts = item.data.numItems;
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
    case VertexDescriptorType::UV4:
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
    case VertexDescriptorType::NORMAL:
    case VertexDescriptorType::NORMAL2:
    case VertexDescriptorType::TANGENT:
    case VertexDescriptorType::TANGENT2:
    case VertexDescriptorType::BONEID:
    case VertexDescriptorType::BONEID2:
    case VertexDescriptorType::VERTEXCOLOR:
    case VertexDescriptorType::VERTEXCOLOR2:
    case VertexDescriptorType::VERTEXCOLOR3:
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
  flags.NoLittleEndian();

  FByteswapper(item);
  item.indices.Fixup(flags.base);
  assert(item.null == 0);

  for (auto &i : item.indices) {
    FByteswapper(i);
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
  flags.NoAutoDetect();
  flags.NoProcessDataOut();

  item.data.items.FixupRelative(flags.base + flags.userData);
  item.descriptors.Fixup(flags.base);
}

template <>
void XN_EXTERN ProcessClass(V3::IndexBuffer &item, ProcessFlags flags) {
  flags.NoBigEndian();
  flags.NoAutoDetect();
  flags.NoProcessDataOut();
  item.indices.items.FixupRelative(flags.base + flags.userData);
}

template <>
void XN_EXTERN ProcessClass(V3::MorphDescriptor &item, ProcessFlags flags) {
  flags.NoBigEndian();
  flags.NoProcessDataOut();
  flags.NoAutoDetect();

  item.morphIDs.Fixup(flags.base);
}

template <>
void XN_EXTERN ProcessClass(V3::MorphsHeader &item, ProcessFlags flags) {
  flags.NoBigEndian();
  flags.NoProcessDataOut();
  flags.NoAutoDetect();

  es::FixupPointers(flags.base, item.descs, item.buffers);

  for (auto &d : item.descs) {
    ProcessClass(d, flags);
  }
}

template <>
void XN_EXTERN ProcessClass(V3::SkinManager &item, ProcessFlags flags) {
  flags.NoBigEndian();
  flags.NoProcessDataOut();
  flags.NoAutoDetect();

  es::FixupPointers(flags.base, item.weightPalettes, item.lodMergeTables);
}

template <> void XN_EXTERN ProcessClass(V3::Stream &item, ProcessFlags flags) {
  flags.NoBigEndian();
  flags.NoProcessDataOut();
  flags.NoAutoDetect();
  flags.base = reinterpret_cast<char *>(&item);

  es::FixupPointers(flags.base, item.vertexBuffers, item.indexBuffers,
                    item.skinManager, item.morphs);

  flags.userData = item.bufferOffset;
  for (auto &v : item.vertexBuffers) {
    ProcessClass(v, flags);
  }

  for (auto &i : item.indexBuffers) {
    ProcessClass(i, flags);
  }

  if (item.skinManager) {
    ProcessClass(*item.skinManager, flags);
  }

  if (item.morphs) {
    ProcessClass(*item.morphs, flags);
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
  flags.base = reinterpret_cast<char *>(&item);
  es::FixupPointers(flags.base, item.nodes, item.nodeIBMs, item.nodeTMs0,
                    item.nodeTMs1);
  assert(item.count1 == item.count2);

  V3::Bone *nodes = item.nodes;

  for (size_t b = 0; b < item.count1; b++) {
    ProcessClass(nodes[b], flags);
  }
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
  flags.base = reinterpret_cast<char *>(&item);
  item.controls.Fixup(flags.base);

  for (auto &c : item.controls) {
    ProcessClass(c, flags);
  }
}

template <> void XN_EXTERN ProcessClass(V3::Model &item, ProcessFlags flags) {
  flags.NoBigEndian();
  flags.NoProcessDataOut();
  flags.NoAutoDetect();
  flags.base = reinterpret_cast<char *>(&item);
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
  assert(!item.cachedTextures);
  assert(!item.streams);
  assert(!item.shaders);

  if (exLoads != Wrap::ExcludeLoad::Model && item.models) {
    ProcessClass(*item.models, flags);
    /*if (item.streams) {
      ProcessClass(*item.streams, flags);
    }*/
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
    // ProcessClass(*item.cachedTextures, flags);
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

MDO::V1Model::V1Model(V1::Header &main) {
  auto model = main.models.Get();
  auto streams = main.streams.Get();
  auto mats = main.materials.Get();

  for (auto &v : streams->vertexBuffers) {
    vertexArrays.storage.emplace_back(v);
  }

  for (auto &v : streams->indexBuffers) {
    indexArrays.storage.emplace_back(v);
  }

  for (auto &m : model->meshes) {
    for (auto &p : m.primitives) {
      primitives.storage.emplace_back(p);
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

MDO::V3Model::V3Model(V3::Model *model, BinReaderRef rd) {
  {
    std::string dBuffer;
    rd.ReadContainer(dBuffer, rd.GetSize());
    DRSM::Header *dhdr = reinterpret_cast<DRSM::Header *>(dBuffer.data());
    ProcessClass(*dhdr);
    DRSM::Resources *resources = dhdr->resources;
    auto &modelEntry =
        resources->streamEntries.items.Get()[resources->modelStreamEntryIndex];
    streamBuffer = resources->streams.items.Get()[0].GetData().substr(
        modelEntry.offset, modelEntry.size);
  }

  stream = reinterpret_cast<V3::Stream *>(streamBuffer.data());
  ProcessClass(*stream, {});

  for (auto &v : stream->vertexBuffers) {
    vertexArrays.storage.emplace_back(v);
  }

  if (stream->morphs) {
    for (auto &d : stream->morphs->descs) {
      auto &v = vertexArrays.storage.at(d.vertexBufferTargetIndex);
      auto bbegin = stream->morphs->buffers.begin() + d.morphBufferBeginIndex;
      auto bbuffer = streamBuffer.data() + stream->bufferOffset;
      v.AppendMorphBuffer(bbegin, bbuffer);

      for (size_t index = 2; auto &m : d.morphIDs) {
        V3Morph mph(bbegin + index, bbuffer);
        mph.targetBuffer = d.vertexBufferTargetIndex;
        mph.index = m;
        morphs.storage.emplace_back(std::move(mph));
        index++;
      }
    }

    for (auto &m : model->morphs->controls) {
      morphNames.emplace_back(m.name1.Get());
    }
  }

  for (auto &v : stream->indexBuffers) {
    indexArrays.storage.emplace_back(v);
  }

  for (auto &m : model->meshes) {
    for (auto &p : m.primitives) {
      primitives.storage.emplace_back(p);
    }
  }

  if (model->skin) {
    V3Skin sk;
    sk.main = model->skin;
    skins.storage.emplace_back(sk);

    if (stream->skinManager) {
      weightSamplers = V3WeightSamplers_t(
          stream->skinManager,
          vertexArrays.storage.at(stream->skinManager->weightBufferID));
    }
  }
}

namespace MXMD {

class Impl {
public:
  std::string buffer;
  std::variant<MDO::Skeleton, MDO::V3Skeleton> skel;
  std::variant<MDO::V1Model, MDO::V3Model> model;

  void Load(BinReaderRef rd, BinReaderRef stream_,
            Wrap::ExcludeLoads excludeLoads) {
    rd.ReadContainer(buffer, rd.GetSize());

    HeaderBase &hdr = reinterpret_cast<HeaderBase &>(*buffer.data());
    ProcessFlags flags{ProcessFlag::AutoDetectEndian};
    flags.userData = static_cast<uint32>(excludeLoads);
    ProcessClass(hdr, flags);

    if (hdr.version == Versions::MXMDVer1) {
      V1::Header &main = static_cast<V1::Header &>(hdr);
      if (main.models) {
        skel = MDO::Skeleton(main.models->bones.items,
                             main.models->bones.numItems);
        model = MDO::V1Model(main);
      }
    } else if (hdr.version == Versions::MXMDVer3) {
      V3::Header &main = static_cast<V3::Header &>(hdr);
      if (main.models) {
        model = MDO::V3Model(main.models, stream_);
        if (main.models->skin) {
          skel = MDO::V3Skeleton(main.models->skin);
        }
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

Wrap::operator const uni::Skeleton *() {
  return std::visit([](auto &item) -> uni::Skeleton * { return &item; },
                    pi->skel);
}
Wrap::operator const Model *() {
  return std::visit([](auto &item) -> Model * { return &item; }, pi->model);
}

class WrapFriend : public Wrap {
public:
  using Wrap::pi;
};

Variant GetVariantFromWrapper(Wrap &wp) {
  return static_cast<WrapFriend &>(wp).pi->GetVariant();
}
} // namespace MXMD
