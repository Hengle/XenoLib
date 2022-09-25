/*  MDO2GLTF
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

#include "datas/app_context.hpp"
#include "datas/binreader_stream.hpp"
#include "datas/binwritter.hpp"
#include "datas/endian.hpp"
#include "datas/except.hpp"
#include "datas/fileinfo.hpp"
#include "datas/matrix44.hpp"
#include "datas/reflector.hpp"
#include "gltf.hpp"
#include "nlohmann/json.hpp"
#include "project.h"
#include "xenolib/bc/skel.hpp"
#include "xenolib/internal/mxmd.hpp"
#include "xenolib/mxmd.hpp"
#include "xenolib/sar.hpp"

es::string_view filters[]{
    ".camdo$",
    ".wimdo$",
    {},
};

static struct MDO2GLTF : ReflectorBase<MDO2GLTF> {
  std::string fallbackSkeletonFilename;
} settings;

REFLECT(CLASS(MDO2GLTF),
        MEMBERNAME(fallbackSkeletonFilename, "fallback-skeleton", "f",
                   ReflDesc{
                       "Fallback skeleton file name if none wasn't found."}), );

static AppInfo_s appInfo{
    AppInfo_s::CONTEXT_VERSION,
    AppMode_e::CONVERT,
    ArchiveLoadType::ALL,
    MDO2GLTF_DESC " v" MDO2GLTF_VERSION ", " MDO2GLTF_COPYRIGHT "Lukas Cone",
    reinterpret_cast<ReflectorFriend *>(&settings),
    filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

struct AABBResult {
  Vector4A16 max;
  Vector4A16 min;
  Vector4A16 center;
};

AABBResult GetAABB(const std::vector<Vector4A16> &points) {
  Vector4A16 max(-INFINITY), min(INFINITY), center;
  for (auto &p : points) {
    max = Vector4A16(_mm_max_ps(max._data, p._data));
    min = Vector4A16(_mm_min_ps(min._data, p._data));
  }
  center = (max + min) * 0.5f;

  return {max, min, center};
}

struct MainGLTF : GLTF {
  void LoadSkeleton(AppContext *ctx) {
    AFileInfo outPath(ctx->workingFile);
    AppContextStream skelArc;
    std::string buffer;

    auto TryFindSkeleton = [&](std::string base) -> char * {
      try {
        skelArc = ctx->RequestFile(base + ".arc");
      } catch (const es::FileNotFoundError &e) {
        try {
          skelArc = ctx->RequestFile(base + ".chr");
        } catch (const es::FileNotFoundError &e) {
          return nullptr;
        }
      }

      BinReaderRef rd(*skelArc.Get());
      {
        SAR::Header base;
        rd.Push();
        rd.Read(base);
        rd.Pop();

        if (base.id == SAR::ID_BIG) {
          FByteswapper(base);
        } else if (base.id != SAR::ID) {
          throw es::InvalidHeaderError(base.id);
        }
      }

      rd.ReadContainer(buffer, rd.GetSize());
      SAR::Header *sar = reinterpret_cast<SAR::Header *>(buffer.data());
      ProcessClass(*sar);

      for (auto &f : sar->entries) {
        es::string_view fname(f.fileName);

        if (fname == "skeleton" || fname.ends_with(".skl")) {
          return f.data;
        }
      }
      return nullptr;
    };

    auto skelData = TryFindSkeleton(outPath.GetFullPathNoExt());

    if (!skelData && !settings.fallbackSkeletonFilename.empty()) {
      skelData = TryFindSkeleton(outPath.GetFolder().to_string() +
                                 settings.fallbackSkeletonFilename);
    }

    if (!skelData) {
      throw std::runtime_error("Cannot find skeleton data");
    }

    BC::Header *hdr = reinterpret_cast<BC::Header *>(skelData);
    ProcessClass(*hdr, {});

    BC::SKEL::Header *sklh = static_cast<BC::SKEL::Header *>(hdr->data);

    if (sklh->id != BC::SKEL::ID) {
      throw es::InvalidHeaderError(sklh->id);
    }

    const size_t nodesBegin = nodes.size();
    scenes.front().nodes.push_back(nodesBegin);
    auto skl = sklh->skel;

    for (size_t index = 0; auto b : skl->boneLinks) {
      gltf::Node node;
      node.name = skl->bones.items[index].name;
      auto &tm = skl->boneTransforms.items[index];
      memcpy(node.translation.data(), &tm.translation,
             sizeof(node.translation));
      memcpy(node.rotation.data(), &tm.rotation, sizeof(node.rotation));
      memcpy(node.scale.data(), &tm.scale, sizeof(node.scale));

      if (b >= 0) {
        nodes.at(b + nodesBegin).children.push_back(index + nodesBegin);
      }

      nodes.emplace_back(std::move(node));
      index++;
    }
  }

  void ProcessSkins(const uni::Model *model, const uni::Skeleton *skel) {
    auto skins = model->Skins();
    auto bones = skel->Bones();

    auto &ibmStream = NewStream("ibms");

    for (auto s : *skins) {
      auto [acc, accId] = NewAccessor(ibmStream, 16);
      acc.componentType = gltf::Accessor::ComponentType::Float;
      acc.count = s->NumNodes();
      acc.type = gltf::Accessor::Type::Mat4;

      gltf::Skin sk;
      sk.inverseBindMatrices = accId;

      for (size_t b = 0; b < s->NumNodes(); b++) {
        es::Matrix44 mtx;
        s->GetTM(mtx, b);
        ibmStream.wr.Write(mtx);
        auto boneName = bones->At(s->NodeIndex(b))->Name();

        sk.joints.push_back([&] {
          for (auto &n : nodes) {
            if (n.name == boneName) {
              return std::distance(nodes.data(), &n);
            }
          }

          throw std::runtime_error("Node " + boneName + " not found");
        }());
      }

      this->skins.emplace_back(std::move(sk));
    }
  }

  void ProcessMeshes(const MXMD::Model *model) {
    auto prims = model->Primitives();
    auto indices = model->Indices();
    auto vertices = model->Vertices();

    struct IndexBuffer {
      uint32 minId;
      uint32 maxId;
      uint32 accIndex;
    };
    std::map<size_t, IndexBuffer> indexBuffers;

    struct VertexBuffer {
      gltf::Attributes main;
      std::vector<gltf::Attributes> morphs;
      size_t numVertices;
      std::vector<uint32> indices;
      std::vector<uint32> boneWts;
      std::vector<uint32> boneIndices;
    };
    std::map<size_t, VertexBuffer> vertexBuffers;

    for (auto p : *prims) {
      if (size_t index = p->IndexArrayIndex(); !indexBuffers.contains(index)) {
        auto ib = indices->At(index);
        auto &istream = GetIndexStream();
        auto [acc, accId] = NewAccessor(istream, ib->IndexSize());
        acc.componentType = ib->IndexSize() == 2
                                ? gltf::Accessor::ComponentType::UnsignedShort
                                : gltf::Accessor::ComponentType::UnsignedInt;
        acc.count = ib->NumIndices();
        acc.type = gltf::Accessor::Type::Scalar;
        istream.wr.WriteBuffer(ib->RawIndexBuffer(),
                               acc.count * ib->IndexSize());

        auto Gather = [&, &acc = acc](auto *ids) {
          using vtype =
              std::remove_const_t<std::remove_pointer_t<decltype(ids)>>;
          vtype minId = vtype(-1);
          vtype maxId = 0;
          for (size_t i = 0; i < acc.count; i++) {
            minId = std::min(ids[i], minId);
            maxId = std::max(ids[i], maxId);
          }

          return std::make_pair(uint32(minId), uint32(maxId));
        };

        auto [minId, maxId] = [&] {
          if (ib->IndexSize() == 2) {
            const uint16 *ids =
                reinterpret_cast<const uint16 *>(ib->RawIndexBuffer());
            return Gather(ids);
          } else {
            const uint32 *ids =
                reinterpret_cast<const uint32 *>(ib->RawIndexBuffer());
            return Gather(ids);
          }
        }();

        indexBuffers.emplace(index, IndexBuffer{minId, maxId, uint32(accId)});
      }

      if (size_t index = p->VertexArrayIndex(0);
          !vertexBuffers.contains(index)) {
        gltf::Attributes attrs;
        auto vb = vertices->At(index);
        const size_t numVerts = vb->NumVertices();
        std::vector<uint32> indices;

        for (auto descs = vb->Descriptors(); auto d : *descs) {
          switch (d->Usage()) {
          case uni::PrimitiveDescriptor::Usage_e::Position: {
            auto &vStream = GetVt12();
            auto [acc, accId] = NewAccessor(vStream, 4);
            acc.componentType = gltf::Accessor::ComponentType::Float;
            acc.type = gltf::Accessor::Type::Vec3;
            acc.count = numVerts;
            attrs["POSITION"] = accId;

            uni::FormatCodec::fvec basePosition;
            d->Codec().Sample(basePosition, d->RawBuffer(), acc.count,
                              d->Stride());
            d->Resample(basePosition);

            auto aabb = GetAABB(basePosition);

            acc.max.insert(acc.max.begin(), aabb.max._arr, aabb.max._arr + 3);
            acc.min.insert(acc.min.begin(), aabb.min._arr, aabb.min._arr + 3);

            for (auto v : basePosition) {
              vStream.wr.Write<Vector>(v);
            }
            break;
          }

          case uni::PrimitiveDescriptor::Usage_e::VertexIndex: {
            indices.resize(numVerts);

            for (size_t v = 0; v < numVerts; v++) {
              IVector4A16 vec;
              d->Codec().GetValue(vec, d->RawBuffer() + v * d->Stride());
              indices.at(v) = vec.X;
            }

            break;
          }

          case uni::PrimitiveDescriptor::Usage_e::Normal: {
            auto &stream = GetVt8();
            auto [acc, index] = NewAccessor(stream, 2);
            acc.count = numVerts;
            acc.componentType = gltf::Accessor::ComponentType::Short;
            acc.normalized = true;
            acc.type = gltf::Accessor::Type::Vec3;
            attrs["NORMAL"] = index;

            uni::FormatCodec::fvec sampled;
            d->Codec().Sample(sampled, d->RawBuffer(), numVerts, d->Stride());
            d->Resample(sampled);

            for (auto v : sampled) {
              v *= Vector4A16(1, 1, 1, 0);
              v.Normalize() *= 0x7fff;
              v = Vector4A16(_mm_round_ps(v._data, _MM_ROUND_NEAREST));
              auto comp = v.Convert<int16>();
              stream.wr.Write(comp);
            }

            break;
          }

          case uni::PrimitiveDescriptor::Usage_e::Tangent: {
            auto &stream = GetVt8();
            auto [acc, index] = NewAccessor(stream, 2);
            acc.count = numVerts;
            acc.componentType = gltf::Accessor::ComponentType::Short;
            acc.normalized = true;
            acc.type = gltf::Accessor::Type::Vec4;
            attrs["TANGENT"] = index;

            uni::FormatCodec::fvec sampled;
            d->Codec().Sample(sampled, d->RawBuffer(), numVerts, d->Stride());
            d->Resample(sampled);

            for (auto v : sampled) {
              v = (v * Vector4A16(1, 1, 1, 0)).Normalized() +
                  v * Vector4A16(0, 0, 0, 1);
              v *= 0x7fff;
              v = Vector4A16(_mm_round_ps(v._data, _MM_ROUND_NEAREST));
              auto comp = v.Convert<int16>();
              stream.wr.Write(comp);
            }

            break;
          }

          case uni::PrimitiveDescriptor::Usage_e::TextureCoordiante: {
            uni::FormatCodec::fvec sampled;
            d->Codec().Sample(sampled, d->RawBuffer(), numVerts, d->Stride());
            d->Resample(sampled);
            auto aabb = GetAABB(sampled);
            auto &max = aabb.max;
            auto &min = aabb.min;
            const bool uv16 = max <= 1.f && min >= -1.f;
            auto &stream = uv16 ? GetVt4() : GetVt8();
            auto [acc, index] = NewAccessor(stream, 4);
            acc.count = numVerts;
            acc.type = gltf::Accessor::Type::Vec2;
            auto coordName = "TEXCOORD_" + std::to_string(d->Index());
            attrs[coordName] = index;

            auto vertWr = stream.wr;

            if (uv16) {
              if (min >= 0.f) {
                acc.componentType =
                    gltf::Accessor::ComponentType::UnsignedShort;
                acc.normalized = true;

                for (auto &v : sampled) {
                  v.Normalize() *= 0xffff;
                  v = Vector4A16(_mm_round_ps(v._data, _MM_ROUND_NEAREST));
                  USVector4 comp = v.Convert<uint16>();
                  vertWr.Write(USVector2(comp));
                }
              } else {
                acc.componentType = gltf::Accessor::ComponentType::Short;
                acc.normalized = true;

                for (auto &v : sampled) {
                  v.Normalize() *= 0x7fff;
                  v = Vector4A16(_mm_round_ps(v._data, _MM_ROUND_NEAREST));
                  SVector4 comp = v.Convert<int16>();
                  vertWr.Write(SVector2(comp));
                }
              }
            } else {
              acc.componentType = gltf::Accessor::ComponentType::Float;

              for (auto &v : sampled) {
                vertWr.Write(Vector2(v));
              }
            }

            break;
          }

          case uni::PrimitiveDescriptor::Usage_e::VertexColor: {
            auto &stream = GetVt4();
            auto [acc, index] = NewAccessor(stream, 1);
            acc.count = numVerts;
            acc.componentType = gltf::Accessor::ComponentType::UnsignedByte;
            acc.normalized = true;
            acc.type = gltf::Accessor::Type::Vec4;
            auto coordName = "COLOR_" + std::to_string(d->Index());
            attrs[coordName] = index;
            uni::FormatCodec::ivec sampled;
            d->Codec().Sample(sampled, d->RawBuffer(), numVerts, d->Stride());

            for (auto &v : sampled) {
              stream.wr.Write(v.Convert<uint8>());
            }

            break;
          }

          default:
            break;
          }
        }

        vertexBuffers.emplace(
            index,
            VertexBuffer{
                std::move(attrs), {}, numVerts, std::move(indices), {}, {}});
      }
    }

    if (auto morphs = model->MorphTargets()) {
      for (auto m : *morphs) {
        auto &vtBuff = vertexBuffers.at(m->TargetVertexArrayIndex());
        gltf::Accessor baseAcc{};
        baseAcc.componentType = gltf::Accessor::ComponentType::Float;
        baseAcc.type = gltf::Accessor::Type::Vec3;
        baseAcc.count = vtBuff.numVertices;

        if (vtBuff.morphs.empty()) {
          gltf::Attributes morphAttrs;
          morphAttrs["POSITION"] = accessors.size();
          baseAcc.max.resize(3);
          baseAcc.min.resize(3);
          accessors.emplace_back(baseAcc);
          baseAcc.max.resize(0);
          baseAcc.min.resize(0);

          vtBuff.morphs.insert(vtBuff.morphs.begin(),
                               model->MorphTargetNames().size(), morphAttrs);
        }

        if (m->NumVertices() == 0) {
          continue;
        }

        auto &vStream = GetSparseStream();
        auto &sparse = baseAcc.sparse;
        sparse.count = m->NumVertices();
        sparse.values.bufferView = vStream.slot;
        sparse.indices.bufferView = vStream.slot;
        sparse.indices.componentType =
            gltf::Accessor::ComponentType::UnsignedShort;

        uni::Element<const uni::PrimitiveDescriptor> normals;
        std::vector<uint16> indices;

        for (auto descs = m->Descriptors(); auto d : *descs) {
          switch (d->Usage()) {
          case uni::PrimitiveDescriptor::Usage_e::PositionDelta: {
            vStream.wr.ApplyPadding(4);
            sparse.values.byteOffset = vStream.wr.Tell();

            uni::FormatCodec::fvec deltaPosition;
            d->Codec().Sample(deltaPosition, d->RawBuffer(), sparse.count,
                              d->Stride());
            d->Resample(deltaPosition);

            for (auto v : deltaPosition) {
              vStream.wr.Write<Vector>(v);
            }

            auto aabb = GetAABB(deltaPosition);
            Vector4A16 null{};
            aabb.max = Vector4A16(_mm_max_ps(aabb.max._data, null._data));
            aabb.min = Vector4A16(_mm_min_ps(aabb.min._data, null._data));
            baseAcc.max.insert(baseAcc.max.begin(), aabb.max._arr,
                               aabb.max._arr + 3);
            baseAcc.min.insert(baseAcc.min.begin(), aabb.min._arr,
                               aabb.min._arr + 3);
            break;
          }

          case uni::PrimitiveDescriptor::Usage_e::VertexIndex: {
            vStream.wr.ApplyPadding(2);
            sparse.indices.byteOffset = vStream.wr.Tell();

            uni::FormatCodec::ivec data;
            d->Codec().Sample(data, d->RawBuffer(), sparse.count, d->Stride());
            indices.reserve(sparse.count);

            for (auto v : data) {
              indices.push_back(v.x);
              vStream.wr.Write<uint16>(v.X);
            }

            break;
          }

          case uni::PrimitiveDescriptor::Usage_e::Normal: {
            normals = std::move(d);
          }

          default:
            break;
          }
        }

        gltf::Attributes morphAttrs;
        morphAttrs["POSITION"] = accessors.size();
        accessors.emplace_back(baseAcc);

        if (normals) {
          baseAcc.min.clear();
          baseAcc.max.clear();
          baseAcc.componentType = gltf::Accessor::ComponentType::Byte;
          baseAcc.normalized = true;
          sparse.values.byteOffset = vStream.wr.Tell();

          uni::FormatCodec::fvec sampled;
          normals->Codec().Sample(sampled, normals->RawBuffer(), sparse.count,
                                  normals->Stride());
          normals->Resample(sampled);

          auto vts = vertices->At(m->TargetVertexArrayIndex());
          std::vector<uint16> normIndices;

          for (auto descs = vts->Descriptors(); auto d : *descs) {
            if (d->Usage() == uni::PrimitiveDescriptor::Usage_e::Normal) {
              uni::FormatCodec::fvec data;
              d->Codec().Sample(data, d->RawBuffer(), vts->NumVertices(),
                                d->Stride());
              d->Resample(data);

              for (int32 i = 0; i < sparse.count; i++) {
                auto v = sampled[i];
                v -= data[indices[i]] * Vector4A16(1, 1, 1, 0);
                v *= Vector4A16(0x7f, 0x7f, 0x7f, 0);
                v = Vector4A16(_mm_round_ps(v._data, _MM_ROUND_NEAREST));

                if (v != Vector4A16{}) {
                  normIndices.push_back(indices[i]);
                  auto comp = Vector(v).Convert<int8>();
                  vStream.wr.Write(comp);
                }
              }

              break;
            }
          }

          if (!normIndices.empty()) {
            if (normIndices.size() != indices.size()) {
              vStream.wr.ApplyPadding(2);
              sparse.indices.byteOffset = vStream.wr.Tell();
              sparse.count = normIndices.size();
              vStream.wr.WriteContainer(normIndices);
            }

            morphAttrs["NORMAL"] = accessors.size();
            accessors.emplace_back(std::move(baseAcc));
          }
        }

        vtBuff.morphs.at(m->Index()) = std::move(morphAttrs);
      }
    }

    union MeshKey {
      uint16 rw{};

      struct {
        int8 lodIndex;
        bool morphed;
      };

      MeshKey() = default;
      MeshKey(int8 lod, bool morph) : lodIndex(lod), morphed(morph) {}

      auto operator<=>(const MeshKey &o) const { return rw <=> o.rw; }
    };

    std::map<MeshKey, gltf::Mesh> lodMeshes;

    for (auto p : *prims) {
      gltf::Primitive prim;
      prim.mode = [&] {
        switch (p->IndexType()) {
        case uni::Primitive::IndexType_e::Triangle:
          return gltf::Primitive::Mode::Triangles;
        case uni::Primitive::IndexType_e::Strip:
          return gltf::Primitive::Mode::TriangleStrip;
        default:
          throw std::runtime_error("Invalid index type");
        }
      }();
      auto &iBuffer = indexBuffers.at(p->IndexArrayIndex());
      prim.indices = iBuffer.accIndex;
      auto &vb = vertexBuffers.at(p->VertexArrayIndex(0));
      prim.attributes = vb.main;
      prim.targets = vb.morphs;

      if (auto wts = model->WeightSamplers()) {
        prim.attributes["JOINTS_0"] = -p->VertexArrayIndex(0) - 1;
        prim.attributes["WEIGHTS_0"] = -p->VertexArrayIndex(0) - 1;

        auto wt = wts->Get(p.get());
        std::vector<uint32> pIndices;
        pIndices.reserve(iBuffer.maxId - iBuffer.minId);

        for (uint32 m = iBuffer.minId; m <= iBuffer.maxId; m++) {
          pIndices.push_back(vb.indices.at(m));
        }

        std::vector<uint32> boneWeights;
        std::vector<uint32> boneIndices;

        wt->Resample(pIndices, boneIndices, boneWeights);

        if (vb.boneIndices.empty()) {
          vb.boneIndices.resize(vb.numVertices);
          vb.boneWts.resize(vb.numVertices);
        }

        for (size_t i = 0; i < pIndices.size(); i++) {
          auto &cwt = vb.boneWts.at(iBuffer.minId + i);
          auto &cbn = vb.boneIndices.at(iBuffer.minId + i);
          if (cwt == 0) {
            cwt = boneWeights.at(i);
            cbn = boneIndices.at(i);
          } else {
            if (cbn != boneIndices.at(i)) {
              assert(false);
            }
            if (cwt != boneWeights.at(i)) {
              assert(false);
            }
          }
        }
      }

      auto &lodMesh =
          lodMeshes[MeshKey{int8(p->LODIndex()), !vb.morphs.empty()}];
      lodMesh.primitives.emplace_back(std::move(prim));
      if (!vb.morphs.empty() &&
          !lodMesh.GetExtensionsAndExtras().contains("extras")) {
        lodMesh.GetExtensionsAndExtras()["extras"]["targetNames"] =
            model->MorphTargetNames();
      }
    }

    std::map<size_t, size_t> wtsMap;

    if (model->WeightSamplers()) {
      for (auto &[vbId, vb] : vertexBuffers) {
        auto &stream = GetVt4();
        auto [acc, index] = NewAccessor(stream, 4);
        acc.count = vb.numVertices;
        acc.componentType = gltf::Accessor::ComponentType::UnsignedByte;
        acc.normalized = true;
        acc.type = gltf::Accessor::Type::Vec4;
        wtsMap[vbId] = index;
        stream.wr.WriteContainer(vb.boneWts);

        auto [accIds, _] = NewAccessor(stream, 4, 0, &acc);
        accIds.normalized = false;
        stream.wr.WriteContainer(vb.boneIndices);
      }
    }

    for (auto &[lod, mesh] : lodMeshes) {
      if (model->WeightSamplers()) {
        for (auto &p : mesh.primitives) {
          if (int32 jid = p.attributes.at("JOINTS_0"); jid < 0) {
            p.attributes.at("JOINTS_0") = wtsMap.at(-jid - 1) + 1;
            p.attributes.at("WEIGHTS_0") = wtsMap.at(-jid - 1);
          }
        }
      }

      mesh.name = "LOD_" + std::to_string(lod.lodIndex);
      if (lod.morphed) {
        mesh.name.append("_morph");
      }
      gltf::Node mNode;
      mNode.name = mesh.name;
      mNode.mesh = meshes.size();
      if (!skins.empty()) {
        mNode.skin = 0;
      }
      meshes.emplace_back(std::move(mesh));
      scenes.back().nodes.push_back(nodes.size());
      nodes.emplace_back(mNode);
    }
  }

  GLTFStream &GetIndexStream() {
    if (indexStream < 0) {
      auto &str = NewStream("indices");
      str.target = gltf::BufferView::TargetType::ElementArrayBuffer;
      indexStream = str.slot;
      return str;
    }

    return Stream(indexStream);
  }

  GLTFStream &GetVt12() {
    if (vt12Stream < 0) {
      auto &str = NewStream("vtStride12", 12);
      str.target = gltf::BufferView::TargetType::ArrayBuffer;
      vt12Stream = str.slot;
      return str;
    }

    return Stream(vt12Stream);
  }

  GLTFStream &GetVt8() {
    if (vt8Stream < 0) {
      auto &str = NewStream("vtStride8", 8);
      str.target = gltf::BufferView::TargetType::ArrayBuffer;
      vt8Stream = str.slot;
      return str;
    }

    return Stream(vt8Stream);
  }

  GLTFStream &GetVt4() {
    if (vt4Stream < 0) {
      auto &str = NewStream("vtStride4", 4);
      str.target = gltf::BufferView::TargetType::ArrayBuffer;
      vt4Stream = str.slot;
      return str;
    }

    return Stream(vt4Stream);
  }

  GLTFStream &GetSparseStream() {
    if (sparsexStream < 0) {
      auto &str = NewStream("sparse");
      sparsexStream = str.slot;
      return str;
    }

    return Stream(sparsexStream);
  }

private:
  int32 indexStream = -1;
  int32 vt12Stream = -1;
  int32 vt8Stream = -1;
  int32 vt4Stream = -1;
  int32 sparsexStream = -1;
};

void AppProcessFile(std::istream &stream, AppContext *ctx) {
  BinReaderRef rd(stream);
  bool isXC = false;
  {
    MXMD::HeaderBase base;
    rd.Push();
    rd.Read(base);
    rd.Pop();

    if (base.magic == MXMD::ID_BIG) {
      FByteswapper(base);
    } else if (base.magic != MXMD::ID) {
      throw es::InvalidHeaderError(base.magic);
    }

    if ((isXC = base.version != MXMD::Versions::MXMDVer1) &&
        base.version != MXMD::Versions::MXMDVer3) {
      throw es::InvalidVersionError(base.version);
    }
  }

  MainGLTF main;
  main.LoadSkeleton(ctx);
  main.extensionsRequired.emplace_back("KHR_mesh_quantization");
  main.extensionsUsed.emplace_back("KHR_mesh_quantization");

  MXMD::Wrap mxmd;

  if (isXC) {
    AFileInfo workingPath(ctx->workingFile);
    AppContextStream stream =
        ctx->RequestFile(workingPath.GetFullPathNoExt().to_string() + ".wismt");
    mxmd.Load(rd, *stream.Get());
  } else {
    mxmd.Load(rd, {});
  }

  const MXMD::Model *model = mxmd;
  const uni::Skeleton *skeleton = mxmd;
  main.ProcessSkins(model, skeleton);
  main.ProcessMeshes(model);

  AFileInfo outPath(ctx->outFile);
  BinWritter wr(outPath.GetFullPathNoExt().to_string() + ".glb");

  main.FinishAndSave(wr, outPath.GetFolder());
}
