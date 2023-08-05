/*  SMExtract
    Copyright(C) 2022-2023 Lukas Cone

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

#include "dds.hpp"
#include "project.h"
#include "spike/app_context.hpp"
#include "spike/except.hpp"
#include "spike/io/binreader_stream.hpp"
#include "spike/io/binwritter_stream.hpp"
#include "spike/master_printer.hpp"
#include "spike/util/aabb.hpp"
#include "xenolib/internal/model.hpp"
#include "xenolib/msmd.hpp"
#include "xenolib/mtxt.hpp"
#include "xenolib/xbc1.hpp"
#include <map>
#include <sstream>

#include "spike/io/vectors.hpp"

std::string_view filters[]{
    ".casmhd$",
    ".wismhd$",
};

static AppInfo_s appInfo{
    .header = SMExtract_DESC " v" SMExtract_VERSION ", " SMExtract_COPYRIGHT
                             "Lukas Cone",
    .filters = filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

void MakeGLTFBufferV1(std::string buffer, AppExtractContext *ctx,
                      TFBH::Builder<TFBH::Header> &imHeader,
                      TFBH::BuilderItem<TFBH::Array<TFBH::Stream>> &imStreams,
                      size_t index) {
  using namespace MXMD::V1;
  auto str = reinterpret_cast<Stream *>(buffer.data());
  ProcessClass(*str, {ProcessFlag::EnsureBigEndian});

  auto Stream = [&]() -> auto & { return imStreams.Get()[index]; };
  auto TotalSize = [&]() -> auto & { return imHeader.Data().bufferSize; };
  auto View = [&](auto at) -> auto & { return Stream().views.at(uint32(at)); };

  auto imIndices =
      imHeader.SetArray(Stream().indices, str->indexBuffers.numItems);

  View(TFBH::VtViewSlot::IndexShort).offset = TotalSize();

  for (size_t iindex = 0; auto &b : str->indexBuffers) {
    TFBH::VtIndex idx{};
    idx.count = b.indices.numItems;
    idx.viewSlot = TFBH::VtViewSlot::IndexShort;
    idx.viewOffset = View(idx.viewSlot).size;

    std::stringstream str;
    BinWritterRef wr(str);
    wr.WriteContainer(std::span<uint16>(b.indices.begin(), b.indices.end()));
    wr.ApplyPadding(4);

    View(idx.viewSlot).size += wr.Tell();
    TotalSize() += wr.Tell();
    imIndices.Get()[iindex++] = idx;
    ctx->SendData(str.str());
  }

  std::vector<MDO::VertexBuffer> mBuffers;
  auto imVtBlocks =
      imHeader.SetArray(Stream().vertices, str->vertexBuffers.numItems);

  for (size_t vIndex = 0; auto &b : str->vertexBuffers) {
    auto &vtBlock = imVtBlocks.Get()[vIndex++];
    auto &ced = mBuffers.emplace_back(b);
    vtBlock.numVertices = ced.NumVertices();
  }

  std::vector<std::vector<TFBH::VtAttr>> mAttrs;
  mAttrs.resize(mBuffers.size());

  View(TFBH::VtViewSlot::Float3).offset = TotalSize();

  for (size_t vIndex = 0; auto &b : mBuffers) {
    auto &vtBlock = imVtBlocks.Get()[vIndex];
    auto &mAttr = mAttrs.at(vIndex++);

    for (auto descs = b.Descriptors(); auto d : *descs) {
      switch (d->Usage()) {
      case uni::PrimitiveDescriptor::Usage_e::Position: {
        TFBH::VtAttr attr{};
        attr.viewSlot = TFBH::VtViewSlot::Float3;
        attr.viewOffset = View(attr.viewSlot).size;
        attr.type = TFBH::VtType::Position;

        uni::FormatCodec::fvec basePosition;
        d->Codec().Sample(basePosition, d->RawBuffer(), b.NumVertices(),
                          d->Stride());
        d->Resample(basePosition);

        auto aabb = GetAABB(basePosition);

        vtBlock.bboxMax = aabb.max;
        vtBlock.bboxMin = aabb.min;

        std::stringstream str;
        BinWritterRef wr(str);

        for (auto v : basePosition) {
          wr.Write<Vector>(v);
        }

        View(attr.viewSlot).size += wr.Tell();
        TotalSize() += wr.Tell();
        mAttr.emplace_back(attr);
        ctx->SendData(str.str());
        break;
      }
      default:
        break;
      }
    }
  }

  View(TFBH::VtViewSlot::Short4).offset = TotalSize();

  for (size_t vIndex = 0; auto &b : mBuffers) {
    auto &mAttr = mAttrs.at(vIndex++);

    for (auto descs = b.Descriptors(); auto d : *descs) {
      switch (d->Usage()) {
      case uni::PrimitiveDescriptor::Usage_e::Normal: {
        TFBH::VtAttr attr{};
        attr.viewSlot = TFBH::VtViewSlot::Short4;
        attr.viewOffset = View(attr.viewSlot).size;
        attr.type = TFBH::VtType::Normal;

        uni::FormatCodec::fvec sampled;
        d->Codec().Sample(sampled, d->RawBuffer(), b.NumVertices(),
                          d->Stride());
        d->Resample(sampled);

        std::stringstream str;
        BinWritterRef wr(str);

        for (auto v : sampled) {
          v *= Vector4A16(1, 1, 1, 0);
          v.Normalize() *= 0x7fff;
          v = Vector4A16(_mm_round_ps(v._data, _MM_ROUND_NEAREST));
          auto comp = v.Convert<int16>();
          wr.Write(comp);
        }

        View(attr.viewSlot).size += wr.Tell();
        TotalSize() += wr.Tell();
        mAttr.emplace_back(attr);
        ctx->SendData(str.str());
        break;
      }

      case uni::PrimitiveDescriptor::Usage_e::Tangent: {
        TFBH::VtAttr attr{};
        attr.viewSlot = TFBH::VtViewSlot::Short4;
        attr.viewOffset = View(attr.viewSlot).size;
        attr.type = TFBH::VtType::Tangent;

        uni::FormatCodec::fvec sampled;
        d->Codec().Sample(sampled, d->RawBuffer(), b.NumVertices(),
                          d->Stride());
        d->Resample(sampled);

        std::stringstream str;
        BinWritterRef wr(str);

        for (auto v : sampled) {
          v = (v * Vector4A16(1, 1, 1, 0)).Normalized() +
              v * Vector4A16(0, 0, 0, 1);
          v *= 0x7fff;
          v = Vector4A16(_mm_round_ps(v._data, _MM_ROUND_NEAREST));
          auto comp = v.Convert<int16>();
          wr.Write(comp);
        }

        View(attr.viewSlot).size += wr.Tell();
        TotalSize() += wr.Tell();
        mAttr.emplace_back(attr);
        ctx->SendData(str.str());
        break;
      }
      default:
        break;
      }
    }
  }

  View(TFBH::VtViewSlot::Float2).offset = TotalSize();
  std::stringstream strUV16;
  BinWritterRef wrUV16(strUV16);

  for (size_t vIndex = 0; auto &b : mBuffers) {
    auto &mAttr = mAttrs.at(vIndex++);

    for (auto descs = b.Descriptors(); auto d : *descs) {
      switch (d->Usage()) {
      case uni::PrimitiveDescriptor::Usage_e::TextureCoordiante: {
        TFBH::VtAttr attr{};
        uni::FormatCodec::fvec sampled;
        d->Codec().Sample(sampled, d->RawBuffer(), b.NumVertices(),
                          d->Stride());
        d->Resample(sampled);
        auto aabb = GetAABB(sampled);
        auto &max = aabb.max;
        auto &min = aabb.min;
        const bool uv16 = max <= 1.f && min >= -1.f;

        if (uv16) {
          if (min >= 0.f) {
            attr.viewSlot = TFBH::VtViewSlot::Short2;
            attr.viewOffset = View(attr.viewSlot).size;
            attr.type = TFBH::VtType::TexCoordUnsigned;

            for (auto &v : sampled) {
              v.Normalize() *= 0xffff;
              v = Vector4A16(_mm_round_ps(v._data, _MM_ROUND_NEAREST));
              USVector4 comp = v.Convert<uint16>();
              wrUV16.Write(USVector2(comp));
            }
          } else {
            attr.viewSlot = TFBH::VtViewSlot::Short2;
            attr.viewOffset = View(attr.viewSlot).size;
            attr.type = TFBH::VtType::TexCoord;

            for (auto &v : sampled) {
              v.Normalize() *= 0x7fff;
              v = Vector4A16(_mm_round_ps(v._data, _MM_ROUND_NEAREST));
              SVector4 comp = v.Convert<int16>();
              wrUV16.Write(SVector2(comp));
            }
          }
        } else {
          attr.viewSlot = TFBH::VtViewSlot::Float2;
          attr.viewOffset = View(attr.viewSlot).size;
          attr.type = TFBH::VtType::TexCoord;

          std::stringstream str;
          BinWritterRef wr(str);

          for (auto &v : sampled) {
            wr.Write(Vector2(v));
          }

          View(attr.viewSlot).size += wr.Tell();
          TotalSize() += wr.Tell();
          ctx->SendData(str.str());
        }

        mAttr.emplace_back(attr);
        break;
      }
      default:
        break;
      }
    }
  }

  if (wrUV16.Tell() > 0) {
    auto &view = View(TFBH::VtViewSlot::Short2);
    view.offset = TotalSize();
    view.size = wrUV16.Tell();
    TotalSize() += view.size;
    ctx->SendData(strUV16.str());
  }

  View(TFBH::VtViewSlot::Byte4).offset = TotalSize();

  for (size_t vIndex = 0; auto &b : mBuffers) {
    auto &mAttr = mAttrs.at(vIndex++);

    for (auto descs = b.Descriptors(); auto d : *descs) {
      switch (d->Usage()) {
      case uni::PrimitiveDescriptor::Usage_e::VertexColor: {
        TFBH::VtAttr attr{};
        attr.viewSlot = TFBH::VtViewSlot::Byte4;
        attr.viewOffset = View(attr.viewSlot).size;
        attr.type = TFBH::VtType::Color;

        uni::FormatCodec::ivec sampled;
        d->Codec().Sample(sampled, d->RawBuffer(), b.NumVertices(),
                          d->Stride());

        std::stringstream str;
        BinWritterRef wr(str);

        for (auto &v : sampled) {
          wr.Write(v.Convert<uint8>());
        }

        View(attr.viewSlot).size += wr.Tell();
        TotalSize() += wr.Tell();
        mAttr.emplace_back(attr);
        ctx->SendData(str.str());
        break;
      }

      default:
        break;
      }
    }
  }

  for (size_t i = 0; i < mBuffers.size(); i++) {
    auto &mAttr = mAttrs.at(i);
    auto vtBlock =
        imHeader.SetArray(imVtBlocks.Get()[i].attributes, mAttr.size());
    auto &imAttr = vtBlock.Get();
    std::copy(mAttr.begin(), mAttr.end(), imAttr.begin());
  }
}

void ExtractV1(MSMD::V1::Header &hdr, AppExtractContext *ctx,
               AppContextStream &stream, std::string mapName) {
  auto GetData = [&](MSMD::StreamEntry entry) {
    std::string buffer;
    buffer.resize(entry.size);
    stream->seekg(entry.offset);
    stream->read(buffer.data(), buffer.size());
    return buffer;
  };

  for (size_t index = 0; auto a : hdr.objectTextures) {
    ctx->NewFile("tex/h/" + mapName + "." + std::to_string(index) + ".dds");
    if (a.highMap.size > 0) {
      auto data = GetData(a.highMap);
      SendTexelsGTX(data, ctx);
    } else {
      auto data = GetData(a.midMap);
      SendTexelsGTX(data, ctx);
    }
    index++;
  }

  auto streamingTextures = hdr.terrainStreamingTextures.items.Get();

  for (size_t index = 0; auto a : hdr.terrainCachedTextures) {
    auto data = GetData(a);
    auto thdr = reinterpret_cast<MSMD::V1::TerrainTextureHeader *>(data.data());
    ProcessClass(*thdr, {ProcessFlag::EnsureBigEndian});

    for (size_t tindex = 0; auto &mt : thdr->textures) {
      ctx->NewFile("tex/m/" + mapName + "." + std::to_string(index) + "." +
                   std::to_string(tindex++) + ".dds");

      if (mt.uncachedID > -1) {
        auto hData = GetData(streamingTextures[mt.uncachedID]);
        SendTexelsGTX(hData, ctx);
      } else {
        SendTexelsGTX({data.data() + mt.offset, mt.size}, ctx);
      }
    }

    index++;
  }

  auto tgldNames = hdr.TGLDNames.items.Get();

  for (size_t index = 0; auto a : hdr.TGLDs) {
    ctx->NewFile(std::string("tgld/") + tgldNames[index].Get() + ".cagld");
    auto data = GetData(a.entry);
    ctx->SendData(data);
    index++;
  }

  for (size_t index = 0; auto a : hdr.skyboxModels) {
    ctx->NewFile("skybox/" + mapName + "_" + std::to_string(index) + ".camdo");
    auto data = GetData(a.entry);
    auto shdr = reinterpret_cast<MSMD::V1::SkyboxHeader *>(data.data());
    FByteswapper(*shdr);
    MXMD::V1::Header nhdr{};
    nhdr.magic = MXMD::ID;
    nhdr.version = MXMD::Versions::MXMDVer1;
    nhdr.models.Reset(shdr->models + 4);
    nhdr.shaders.Reset(shdr->shaders + 4);
    nhdr.streams.Reset(shdr->streams + 4);
    nhdr.cachedTextures.Reset(shdr->cachedTextures + 4);
    nhdr.materials.Reset(shdr->materials + 4);
    nhdr.unk00 = shdr->unk00;
    FByteswapper(nhdr);
    ctx->SendData({reinterpret_cast<char *>(&nhdr), sizeof(nhdr)});
    ctx->SendData(
        {reinterpret_cast<char *>(shdr + 1), data.size() - sizeof(*shdr)});
    index++;
  }

  for (size_t index = 0; auto a : hdr.objects) {
    ctx->NewFile("oj/" + mapName + "_" + std::to_string(index) + ".caim");
    auto data = GetData(a.entry);
    ctx->SendData(data);
    index++;
  }

  try {
    ctx->NewFile("oj/" + mapName + "_oj.bin");
    TFBH::Builder<TFBH::Header> imHeader;
    auto imStreams =
        imHeader.SetArray(imHeader.Data().streams, hdr.objectStreams.numItems);

    for (size_t index = 0; auto a : hdr.objectStreams) {
      MakeGLTFBufferV1(GetData(a), ctx, imHeader, imStreams, index++);
    }

    ctx->NewFile("oj/" + mapName + "_oj.binh");
    ctx->SendData(imHeader.buffer);
  } catch (const std::exception &e) {
    printerror("Failed to extract map objects stream, reason: " << e.what());
  }

  for (size_t index = 0; auto a : hdr.terrainModels) {
    ctx->NewFile("terrain/" + mapName + "_" + std::to_string(index) + ".catm");
    auto data = GetData(a.entry);
    ctx->SendData(data);
    index++;
  }

  try {
    ctx->NewFile("terrain/" + mapName + "_terrain.bin");
    TFBH::Builder<TFBH::Header> imHeader;
    auto imStreams = imHeader.SetArray(imHeader.Data().streams,
                                       hdr.mapTerrainBuffers.numItems);

    for (size_t index = 0; auto a : hdr.mapTerrainBuffers) {
      MakeGLTFBufferV1(GetData(a), ctx, imHeader, imStreams, index++);
    }

    ctx->NewFile("terrain/" + mapName + "_terrain.binh");
    ctx->SendData(imHeader.buffer);
  } catch (const std::exception &e) {
    printerror("Failed to extract map terrain stream, reason: " << e.what());
  }

  for (size_t index = 0; auto a : hdr.terrainLODModels) {
    ctx->NewFile("landmark/" + mapName + "_" + std::to_string(index) +
                 ".camdo");
    auto data = GetData(a.entry);
    auto shdr = reinterpret_cast<MSMD::V1::LandmarkHeader *>(data.data());
    FByteswapper(*shdr);
    MXMD::V1::Header nhdr{};
    nhdr.magic = MXMD::ID;
    nhdr.version = MXMD::Versions::MXMDVer1;
    nhdr.models.Reset(shdr->models - 4);
    nhdr.shaders.Reset(shdr->shaders - 4);
    nhdr.streams.Reset(shdr->streams - 4);
    nhdr.cachedTextures.Reset(shdr->cachedTextures - 4);
    nhdr.materials.Reset(shdr->materials - 4);
    nhdr.unk00 = shdr->unk00;
    FByteswapper(nhdr);
    ctx->SendData({reinterpret_cast<char *>(&nhdr), sizeof(nhdr)});
    ctx->SendData(
        {reinterpret_cast<char *>(shdr + 1), data.size() - sizeof(*shdr)});
    index++;
  }

  auto hkNames = hdr.havokNames.Get();

  for (auto a : hdr.hkColisions) {
    ctx->NewFile("collision/" + std::string(hkNames + a.nameOffset) + "hkx");
    auto data = GetData(a.entry);
    ctx->SendData(data);
  }

  for (size_t index = 0; auto a : hdr.grass) {
    ctx->NewFile("grass/" + mapName + "_" + std::to_string(index) + ".cagmdo");
    auto data = GetData(a.entry);
    ctx->SendData(data);
    index++;
  }

  for (size_t index = 0; auto a : hdr.effects) {
    ctx->NewFile("effect/" + mapName + "_" + std::to_string(index) + ".epac");
    auto data = GetData(a);
    ctx->SendData(data);
    index++;
  }

  for (size_t index = 0; auto a : hdr.unk00) {
    ctx->NewFile("unk00/" + mapName + "_" + std::to_string(index));
    auto data = GetData(a);
    ctx->SendData(data);
    index++;
  }

  for (size_t index = 0; auto a : hdr.unk01) {
    ctx->NewFile("unk01/" + mapName + "_" + std::to_string(index));
    auto data = GetData(a);
    ctx->SendData(data);
    index++;
  }

  for (size_t index = 0; auto a : hdr.unk02) {
    ctx->NewFile("unk02/" + mapName + "_" + std::to_string(index));
    auto data = GetData(a);
    ctx->SendData(data);
    index++;
  }

  ctx->NewFile(mapName + ".calcmd");
  ctx->SendData({hdr.LCMD.data, hdr.LCMD.size});
  ctx->NewFile(mapName + ".cagld");
  ctx->SendData({hdr.cachedTGLD.Get(), hdr.CEMS.Get()});
  ctx->NewFile(mapName + ".cacems");
  ctx->SendData(
      {hdr.CEMS.Get(), reinterpret_cast<const char *>(hdr.BVSC.items.Get())});
  ctx->NewFile(mapName + ".bvsc");
  MSMD::V1::BVSCBlock *bvs = hdr.BVSC.items;
  ctx->SendData({bvs->main.data, bvs->main.size});

  for (auto &b : bvs->entries) {
    ctx->NewFile("bvsc/" + mapName + "_" + std::to_string(b.index) + ".bvsc");
    ctx->SendData({b.data.data, b.data.size});
  }
}

void ExtractV2(MSMD::V2::Header &hdr, AppExtractContext *ctx,
               AppContextStream &stream, std::string mapName) {
  auto GetData = [&](MSMD::StreamEntry entry) {
    std::string buffer;
    buffer.resize(entry.size);
    stream->seekg(entry.offset);
    stream->read(buffer.data(), buffer.size());
    stream->clear();
    return DecompressXBC1(buffer.data());
  };

  for (size_t index = 0; auto a : hdr.objectTextures) {
    ctx->NewFile("tex/h/" + mapName + "." + std::to_string(index) + ".dds");
    auto data = GetData(a.midMap);
    if (a.highMap.size > 0) {
      data.insert(0, GetData(a.highMap));
      auto hdr = const_cast<LBIM::Header *>(LBIM::Mount(data));
      hdr->width *= 2;
      hdr->height *= 2;
    }

    SendTexelsLB(data, ctx);
    index++;
  }
}

void AppProcessFile(AppContext *ctx) {
  std::string dataFile(ctx->workingFile.GetFullPath());
  dataFile.replace(dataFile.size() - 2, 2, "da");
  auto dataStream = ctx->RequestFile(dataFile);

  std::string buffer = ctx->GetBuffer();

  auto hdr = reinterpret_cast<MSMD::HeaderBase *>(buffer.data());
  ProcessClass(*hdr);
  auto ectx = ctx->ExtractContext();

  if (ectx->RequiresFolders()) {
    static const char *folders[]{
        "tex/m/", "tex/h/", "tgld",  "skybox", "collision", "grass",
        "effect", "unk00",  "unk01", "unk02",  "bvsc",      "landmark",
    };

    for (auto f : folders) {
      ectx->AddFolderPath(f);
    }

    ectx->GenerateFolders();
  }

  if (hdr->version == MSMD::Version::V10011) {
    ExtractV1(static_cast<MSMD::V1::Header &>(*hdr), ectx, dataStream,
              std::string(ctx->workingFile.GetFilename()));
  } else {
    ExtractV2(static_cast<MSMD::V2::Header &>(*hdr), ectx, dataStream,
              std::string(ctx->workingFile.GetFilename()));
  }
}
