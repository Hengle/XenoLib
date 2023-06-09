/*  IM2GLTF
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

#include "tfbh.hpp"

#include "datas/app_context.hpp"
#include "datas/binreader_stream.hpp"
#include "datas/endian.hpp"
#include "datas/except.hpp"
#include "datas/matrix44.hpp"
#include "datas/reflector.hpp"
#include "gltf.hpp"
#include "nlohmann/json.hpp"
#include "project.h"
#include "uni/rts.hpp"
#include "xenolib/msim.hpp"
#include <cstdlib>

using namespace fx;

std::string_view filters[]{
    ".caim$",
    ".wiim$",
};

static AppInfo_s appInfo{
    .header =
        IM2GLTF_DESC " v" IM2GLTF_VERSION ", " IM2GLTF_COPYRIGHT "Lukas Cone",
    .filters = filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

struct IMGLTF : GLTF {
  GLTFStream &GetTranslations() {
    if (instTrs < 0) {
      auto &str = NewStream("instance-tms", 20);
      instTrs = str.slot;
      return str;
    }

    return Stream(instTrs);
  }

  GLTFStream &GetScales() {
    if (instScs < 0) {
      auto &str = NewStream("instance-scale");
      instScs = str.slot;
      return str;
    }

    return Stream(instScs);
  }

  gltf::Node &LodNode(int32 lod) {
    if (lodNodes[lod] < 0) {
      scenes.back().nodes.push_back(nodes.size());
      lodNodes[lod] = nodes.size();
      nodes.emplace_back().name = "LOD_" + std::to_string(lod);
    }

    return nodes.at(lodNodes[lod]);
  }

private:
  int32 instTrs = -1;
  int32 instScs = -1;
  int32 lodNodes[4]{-1, -1, -1, -1};
};

void ProcessMeshes(IMGLTF &main, const uni::Model *mod,
                   std::pair<std::vector<gltf::Attributes>, size_t> &attrs) {
  std::map<int8, gltf::Mesh> lodMeshes;

  for (auto prims = mod->Primitives(); auto p : *prims) {
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
    prim.indices = p->IndexArrayIndex() + attrs.second;
    prim.attributes = attrs.first.at(p->VertexArrayIndex(0));

    auto &lodMesh = lodMeshes[int8(p->LODIndex())];
    lodMesh.primitives.emplace_back(std::move(prim));
  }
  gltf::Node modNode;

  if (lodMeshes.size() > 1) {
    for (auto &[lod, mesh] : lodMeshes) {
      mesh.name = "LOD_" + std::to_string(lod);
      gltf::Node mNode;
      mNode.name = mesh.name;
      mNode.mesh = main.meshes.size();
      main.meshes.emplace_back(std::move(mesh));
      modNode.children.push_back(main.nodes.size());
      main.nodes.emplace_back(mNode);
    }
  } else {
    auto cMesh = lodMeshes.begin();
    if (!cMesh->second.name.empty()) {
      modNode.name = cMesh->second.name;
    }

    modNode.mesh = main.meshes.size();
    main.meshes.emplace_back(std::move(cMesh->second));

    main.LodNode(cMesh->first).children.emplace_back(main.nodes.size());
  }

  if (auto skins = mod->Skins(); skins->Size() > 0) {
    auto skin = skins->At(0);

    if (skin->NumNodes() == 1) {
      es::Matrix44 mtx;
      skin->GetTM(mtx, 0);
      memcpy(modNode.matrix.data(), &mtx, sizeof(mtx));
    } else {
      std::vector<Vector> scales;
      bool processScales = false;

      auto &str = main.GetTranslations();
      auto [accPos, accPosIndex] = main.NewAccessor(str, 4);
      accPos.type = gltf::Accessor::Type::Vec3;
      accPos.componentType = gltf::Accessor::ComponentType::Float;
      accPos.count = skin->NumNodes();

      auto [accRot, accRotIndex] = main.NewAccessor(str, 4, 12);
      accRot.type = gltf::Accessor::Type::Vec4;
      accRot.componentType = gltf::Accessor::ComponentType::Short;
      accRot.normalized = true;
      accRot.count = skin->NumNodes();
      Vector4A16::SetEpsilon(0.00001f);

      for (size_t m = 0; m < accRot.count; m++) {
        es::Matrix44 mtx;
        skin->GetTM(mtx, m);
        uni::RTSValue val{};
        mtx.Decompose(val.translation, val.rotation, val.scale);
        scales.emplace_back(val.scale);

        if (!processScales) {
          processScales = val.scale != Vector4A16(1, 1, 1, 0);
        }

        str.wr.Write<Vector>(val.translation);

        val.rotation.Normalize() *= 0x7fff;
        val.rotation =
            Vector4A16(_mm_round_ps(val.rotation._data, _MM_ROUND_NEAREST));
        auto comp = val.rotation.Convert<int16>();
        str.wr.Write(comp);
      }

      auto &attrs =
          modNode
              .GetExtensionsAndExtras()["extensions"]["EXT_mesh_gpu_instancing"]
                                       ["attributes"];

      attrs["TRANSLATION"] = accPosIndex;
      attrs["ROTATION"] = accRotIndex;

      if (processScales) {
        auto &str = main.GetScales();
        auto [accScale, accScaleIndex] = main.NewAccessor(str, 4);
        accScale.type = gltf::Accessor::Type::Vec3;
        accScale.componentType = gltf::Accessor::ComponentType::Float;
        accScale.count = skin->NumNodes();
        str.wr.WriteContainer(scales);
        attrs["SCALE"] = accScaleIndex;
      }
    }
  }

  main.nodes.emplace_back(modNode);
}

auto ProcessBuffers(gltf::Document &main, const TFBH::Stream &stream) {
  std::vector<gltf::Attributes> buffers;
  size_t beginIndices = main.accessors.size();
  std::map<TFBH::VtViewSlot, size_t> views;

  for (uint32 index = 0; auto &w : stream.views) {
    if (w.size > 0) {
      auto slot = TFBH::VtViewSlot(index);
      gltf::BufferView view{};
      view.buffer = 1;
      view.byteLength = w.size;
      view.byteOffset = w.offset;
      view.byteStride = TFBH::GetStride(slot);
      view.target = [slot] {
        switch (slot) {
        case TFBH::VtViewSlot::IndexInt:
        case TFBH::VtViewSlot::IndexShort:
          return gltf::BufferView::TargetType::ElementArrayBuffer;
        default:
          return gltf::BufferView::TargetType::ArrayBuffer;
        }
      }();

      views.emplace(slot, main.bufferViews.size());
      main.bufferViews.emplace_back(view);
    }

    index++;
  }

  auto AccFromSlot = [&](TFBH::VtViewSlot slot) {
    gltf::Accessor acc{};
    acc.bufferView = views.at(slot);
    using S = decltype(slot);
    switch (slot) {
    case S::Byte4:
      acc.componentType = gltf::Accessor::ComponentType::UnsignedByte;
      acc.type = gltf::Accessor::Type::Vec4;
      acc.normalized = true;
      break;
    case S::Float2:
      acc.componentType = gltf::Accessor::ComponentType::Float;
      acc.type = gltf::Accessor::Type::Vec2;
      break;
    case S::Float3:
      acc.componentType = gltf::Accessor::ComponentType::Float;
      acc.type = gltf::Accessor::Type::Vec3;
      break;
    case S::IndexInt:
      acc.componentType = gltf::Accessor::ComponentType::UnsignedInt;
      acc.type = gltf::Accessor::Type::Scalar;
      break;
    case S::IndexShort:
      acc.componentType = gltf::Accessor::ComponentType::UnsignedShort;
      acc.type = gltf::Accessor::Type::Scalar;
      break;
    case S::Short2:
      acc.componentType = gltf::Accessor::ComponentType::Short;
      acc.type = gltf::Accessor::Type::Vec2;
      acc.normalized = true;
      break;
    case S::Short4:
      acc.componentType = gltf::Accessor::ComponentType::Short;
      acc.type = gltf::Accessor::Type::Vec4;
      acc.normalized = true;
      break;
    default:
      break;
    }

    return acc;
  };

  for (auto &i : stream.indices) {
    gltf::Accessor acc{AccFromSlot(i.viewSlot)};
    acc.byteOffset = i.viewOffset;
    acc.count = i.count;
    main.accessors.emplace_back(std::move(acc));
  }

  for (auto &v : stream.vertices) {
    gltf::Attributes attrs;
    size_t colorIndex = 0;
    size_t uvIndex = 0;

    for (auto &a : v.attributes) {
      gltf::Accessor acc{AccFromSlot(a.viewSlot)};
      acc.byteOffset = a.viewOffset;
      acc.count = v.numVertices;

      auto GetSlot = [&]() -> std::string {
        switch (a.type) {
        case TFBH::VtType::Position:
          return "POSITION";
        case TFBH::VtType::Normal:
          acc.type = gltf::Accessor::Type::Vec3;
          return "NORMAL";
        case TFBH::VtType::Tangent:
          return "TANGENT";
        case TFBH::VtType::Color:
          return "COLOR_" + std::to_string(colorIndex++);
        case TFBH::VtType::TexCoordUnsigned:
          acc.componentType = gltf::Accessor::ComponentType::UnsignedShort;
          [[fallthrough]];
        case TFBH::VtType::TexCoord:
          return "TEXCOORD_" + std::to_string(uvIndex++);
        }

        throw std::runtime_error("Invalid VtType");
      };

      attrs[GetSlot()] = main.accessors.size();

      if (a.type == TFBH::VtType::Position) {
        acc.max.insert(acc.max.begin(), v.bboxMax._arr, v.bboxMax._arr + 3);
        acc.min.insert(acc.min.begin(), v.bboxMin._arr, v.bboxMin._arr + 3);
      }

      main.accessors.emplace_back(std::move(acc));
    }

    buffers.emplace_back(std::move(attrs));
  }

  return std::make_pair(buffers, beginIndices);
}

void AppProcessFile(AppContext *ctx) {
  IMGLTF main;
  main.extensionsRequired.emplace_back("KHR_mesh_quantization");
  main.extensionsUsed.emplace_back("KHR_mesh_quantization");
  gltf::Buffer shared;
  std::string binhBuffer;
  {
    std::string lookupPath(ctx->workingFile.GetFolder());
    lookupPath.append(ctx->workingFile.GetFilename().substr(
        0, ctx->workingFile.GetFilename().find('_')));
    lookupPath.append("_oj.binh");
    auto binStream = ctx->RequestFile(lookupPath);
    lookupPath.pop_back();
    AFileInfo uriPath(lookupPath);
    shared.uri = uriPath.GetFilenameExt();
    BinReaderRef rdb(*binStream.Get());
    rdb.ReadContainer(binhBuffer, rdb.GetSize());
  }

  auto sHdr = reinterpret_cast<const TFBH::Header *>(binhBuffer.data());
  if (sHdr->id != TFBH::ID) {
    throw es::InvalidHeaderError(sHdr->id);
  }

  if (sHdr->version != 1) {
    throw es::InvalidVersionError(sHdr->version);
  }

  shared.byteLength = sHdr->bufferSize;
  main.buffers.emplace_back();
  main.buffers.emplace_back(shared);

  MSIM::Wrap msim;
  BinReaderRef rd(ctx->GetStream());

  if (ctx->workingFile.GetExtension() == ".caim") {
    msim.LoadV1(rd, {MSIM::Wrap::ExcludeLoad::Shaders});
  } else if (ctx->workingFile.GetExtension() == ".wiim") {
    msim.LoadV2(rd, {MSIM::Wrap::ExcludeLoad::Shaders});
  } else {
    throw std::runtime_error("Unsupported format");
  }

  const uni::List<const uni::Model> *models = msim;
  std::map<size_t, std::pair<std::vector<gltf::Attributes>, size_t>> attrs;

  for (size_t index = 0; auto m : *models) {
    size_t mIndex = msim.ModelIndex(index++);

    if (!attrs.contains(mIndex)) {
      attrs.emplace(mIndex, ProcessBuffers(main, sHdr->streams[mIndex]));
    }

    ProcessMeshes(main, m.get(), attrs.at(mIndex));
  }

  if (main.NumStreams() > 0) {
    main.extensionsRequired.emplace_back("EXT_mesh_gpu_instancing");
    main.extensionsUsed.emplace_back("EXT_mesh_gpu_instancing");

    main.FinishAndSave(
        ctx->NewFile(ctx->workingFile.ChangeExtension(".glb")).str, {});
  } else {
    main.buffers.erase(main.buffers.begin());

    for (auto &w : main.bufferViews) {
      w.buffer = 0;
    }

    gltf::Save(main,
               ctx->NewFile(ctx->workingFile.ChangeExtension(".gltf")).str, {},
               false);
  }
}
