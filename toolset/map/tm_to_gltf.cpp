/*  TM2GLTF
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
#include "datas/binwritter_stream.hpp"
#include "datas/endian.hpp"
#include "datas/except.hpp"
#include "datas/reflector.hpp"
#include "gltf.hpp"
#include "project.h"
#include "xenolib/mstm.hpp"
#include <cstdlib>

using namespace fx;

std::string_view filters[]{
    ".catm$",
    ".witm$",
};

static AppInfo_s appInfo{
    .header =
        TM2GLTF_DESC " v" TM2GLTF_VERSION ", " TM2GLTF_COPYRIGHT "Lukas Cone",
    .filters = filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

struct TMGLTF : GLTF {
  gltf::Node &LodNode(int32 lod) {
    if (lodNodes[lod] < 0) {
      scenes.back().nodes.push_back(nodes.size());
      lodNodes[lod] = nodes.size();
      nodes.emplace_back().name = "LOD_" + std::to_string(lod);
    }

    return nodes.at(lodNodes[lod]);
  }

private:
  int32 lodNodes[4]{-1, -1, -1, -1};
};

void ProcessMeshes(TMGLTF &main, const uni::Model *mod,
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
      view.buffer = 0;
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
  TMGLTF main;
  main.extensionsRequired.emplace_back("KHR_mesh_quantization");
  main.extensionsUsed.emplace_back("KHR_mesh_quantization");
  gltf::Buffer shared;
  std::string binhBuffer;
  {
    std::string lookupPath(ctx->workingFile.GetFolder());
    lookupPath.append(ctx->workingFile.GetFilename().substr(
        0, ctx->workingFile.GetFilename().find('_')));
    lookupPath.append("_terrain.binh");
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
  main.buffers.emplace_back(shared);

  MSTM::Wrap mstm;
  BinReaderRef rd(ctx->GetStream());

  if (ctx->workingFile.GetExtension() == ".catm") {
    mstm.LoadV1(rd, {MSTM::Wrap::ExcludeLoad::Shaders});
  } else if (ctx->workingFile.GetExtension() == ".witm") {
    mstm.LoadV2(rd, {MSTM::Wrap::ExcludeLoad::Shaders});
  } else {
    throw std::runtime_error("Unsupported format");
  }

  const uni::List<const uni::Model> *models = mstm;
  std::map<size_t, std::pair<std::vector<gltf::Attributes>, size_t>> attrs;

  for (size_t index = 0; auto m : *models) {
    size_t mIndex = mstm.ModelIndex(index++);

    if (!attrs.contains(mIndex)) {
      attrs.emplace(mIndex, ProcessBuffers(main, sHdr->streams[mIndex]));
    }

    ProcessMeshes(main, m.get(), attrs.at(mIndex));
  }

  gltf::Save(main, ctx->NewFile(ctx->workingFile.ChangeExtension(".gltf")).str,
             {}, false);
}
