/*  SMTExtract
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
#include "datas/except.hpp"
#include "datas/fileinfo.hpp"
#include "dds.hpp"
#include "project.h"
#include "xenolib/drsm.hpp"
#include "xenolib/internal/mxmd.hpp"
#include "xenolib/mxmd.hpp"
#include "xenolib/xbc1.hpp"
#include <set>

es::string_view filters[]{
    ".wismt$",
    ".casmt$",
    ".pcsmt$",
    {},
};

static constexpr bool DEV_EXTRACT_MODEL = false;
static constexpr bool DEV_EXTRACT_ALL = false;

static AppInfo_s appInfo{
    AppInfo_s::CONTEXT_VERSION,
    AppMode_e::EXTRACT,
    ArchiveLoadType::FILTERED,
    SMTExtract_DESC " v" SMTExtract_VERSION ", " SMTExtract_COPYRIGHT
                    "Lukas Cone",
    nullptr,
    filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

struct LocalCache {
  std::string data[2];

  const char *GetSet(DRSM::Stream &stream, size_t slot) {
    if (data[slot].empty()) {
      data[slot] = stream.GetData();
    }

    return data[slot].data();
  }
};

template <class C> void SendDDS(const C *hdr, AppExtractContext *ctx) {
  DDS ddtex = ToDDS(hdr);

  ctx->SendData({reinterpret_cast<const char *>(&ddtex), ddtex.DDS_SIZE});
};

void SendTexelsLB(es::string_view texData, AppExtractContext *ctx) {
  std::string mipOut;
  mipOut.resize(texData.size());
  auto tex = LBIM::Mount(texData);
  SendDDS(tex, ctx);
  LBIM::DecodeMipmap(*tex, texData.data(), mipOut.data());
  ctx->SendData(mipOut);
}

void SendTexelsGTX(es::string_view texData, AppExtractContext *ctx) {
  std::string mipOut;
  mipOut.resize(texData.size());
  auto tex = MTXT::Mount(texData);
  FByteswapper(*const_cast<MTXT::Header *>(tex));
  SendDDS(tex, ctx);
  MTXT::DecodeMipmap(*tex, texData.data(), mipOut.data());
  ctx->SendData(mipOut);
}

void TryExtractDRSM(BinReaderRef rd, AppExtractContext *ctx) {
  {
    DRSM::Header hdr;
    rd.Push();
    rd.Read(hdr);
    rd.Pop();

    if (hdr.id != DRSM::ID) {
      throw es::InvalidHeaderError(hdr.id);
    }
  }

  std::string buffer;
  rd.ReadContainer(buffer, rd.GetSize());

  DRSM::Header *hdr = reinterpret_cast<DRSM::Header *>(buffer.data());
  ProcessClass(*hdr);
  DRSM::Resources *resources = hdr->resources;
  DRSM::Textures *textures = resources->textures;
  DRSM::Stream *streams = resources->streams.items;
  DRSM::StreamEntry *entries = resources->streamEntries.items;
  LocalCache cache;

  if constexpr (DEV_EXTRACT_ALL) {
    for (size_t index = 0; auto &s : resources->streams) {
      ctx->NewFile(std::to_string(index++));
      auto buffer = s.GetData();
      ctx->SendData(buffer);
    }

    return;
  }

  if constexpr (DEV_EXTRACT_MODEL) {
    const char *main = cache.GetSet(*streams, 0);

    {
      ctx->NewFile("vertexindexbuffer");
      auto &entry = entries[resources->modelStreamEntryIndex];
      es::string_view viStream(main + entry.offset, entry.size);
      ctx->SendData(viStream);
    }

    {
      ctx->NewFile("shaders.wishp");
      auto &entry = entries[resources->shaderStreamEntryIndex];
      es::string_view shStream(main + entry.offset, entry.size);
      ctx->SendData(shStream);
    }

    if (!textures) {
      return;
    }
  }

  if (!textures) {
    throw std::runtime_error("No files.");
  }

  uint32 numProcessedTextures = 0;

  for (int32 index = 0; auto &t : textures->textures) {
    int32 hIndex = -1;

    for (int32 index_ = 0; auto h : resources->textureIndices) {
      if (h == index) {
        hIndex = index_;
        break;
      }
      index_++;
    }

    index++;

    if (hIndex > -1) [[likely]] {
      if (resources->externalResources.numItems) {
        continue;
      }
      auto &entry =
          entries[resources->middleTexturesStreamEntryBeginIndex + hIndex];
      auto middleTextures =
          cache.GetSet(streams[resources->middleTexturesStreamIndex], 1);

      es::string_view miMip(middleTextures + entry.offset, entry.size);
      auto tex = LBIM::Mount(miMip);
      ctx->NewFile(t.name.Get() + std::string(".dds"));
      numProcessedTextures++;

      if (entry.unkIndex > resources->middleTexturesStreamIndex) {
        std::string hiMip =
            streams[entry.unkIndex - resources->middleTexturesStreamIndex]
                .GetData();
        std::string hiMipOut;
        hiMipOut.resize(hiMip.size());
        auto texCopy = *tex;
        texCopy.width *= 2;
        texCopy.height *= 2;
        SendDDS(&texCopy, ctx);
        LBIM::DecodeMipmap(texCopy, hiMip.data(), hiMipOut.data());
        ctx->SendData(hiMipOut);
      } else {
        std::string miMipOut;
        miMipOut.resize(miMip.size());
        SendDDS(tex, ctx);
        LBIM::DecodeMipmap(*tex, miMip.data(), miMipOut.data());
        ctx->SendData(miMipOut);
      }

    } else {
      numProcessedTextures++;
      ctx->NewFile(t.name.Get() + std::string(".dds"));
      auto &entry = entries[resources->lowTexturesStreamEntryIndex];
      auto loMips = cache.GetSet(streams[resources->lowTexturesStreamIndex], 0);
      es::string_view loMip(loMips + entry.offset + t.lowOffset, t.lowSize);

      if (loMip.begins_with("DDS")) {
        ctx->SendData(loMip);
      } else {
        std::string loMipOut;
        loMipOut.resize(loMip.size());
        auto tex = LBIM::Mount(loMip);
        SendDDS(tex, ctx);
        LBIM::DecodeMipmap(*tex, loMip.data(), loMipOut.data());
        ctx->SendData(loMipOut);
      }
    }
  }

  if (!numProcessedTextures) {
    throw std::runtime_error("No files.");
  }
}

void ExtractSimple(BinReaderRef rd, MXMD::V1::SMTHeader *hdr,
                   DRSM::Textures *cachedTextures, AppExtractContext *ctx,
                   void (*dataSender)(es::string_view texData,
                                      AppExtractContext *ctx)) {
  std::set<es::string_view> processedTextures;

  if (hdr) {
    for (int32 g = hdr->numUsedGroups - 1; g >= 0; g--) {
      auto textures = hdr->groups[g].Get();
      std::string buffer;
      rd.Seek(hdr->dataOffsets[g]);
      rd.ReadContainer(buffer, hdr->dataSizes[g]);

      for (auto &t : textures->textures) {
        es::string_view tName(t.name.Get());
        if (processedTextures.contains(tName)) {
          continue;
        }

        processedTextures.emplace(tName);
        ctx->NewFile(tName.to_string() + ".dds");
        es::string_view texData(buffer.data() + t.lowOffset, t.lowSize);
        dataSender(texData, ctx);
      }
    }
  }

  if (!cachedTextures) {
    return;
  }

  for (auto &t : cachedTextures->textures) {
    es::string_view tName(t.name.Get());
    if (processedTextures.contains(tName)) {
      continue;
    }

    ctx->NewFile(tName.to_string() + ".dds");

    es::string_view texData(
        reinterpret_cast<char *>(cachedTextures) + t.lowOffset, t.lowSize);
    dataSender(texData, ctx);
  }
}

void ExtractSimple(BinReaderRef rd, MXMD::V3::SMTHeader *hdr,
                   DRSM::Textures *lowTextures, AppExtractContext *ctx) {
  std::set<es::string_view> processedTextures;

  if (hdr) {
    for (int32 g = hdr->numUsedGroups - 1; g >= 0; g--) {
      auto textures = hdr->groups[g].Get();
      std::string buffer = [rd, ctx, g, hdr] {
        std::string buffer;
        rd.Seek(hdr->dataOffsets[g]);
        rd.ReadContainer(buffer, hdr->compressedSize[g]);
        return DecompressXBC1(buffer.data());
      }();

      for (auto &t : textures->textures) {
        es::string_view tName(t.name.Get());
        if (processedTextures.contains(tName)) {
          continue;
        }

        processedTextures.emplace(tName);
        ctx->NewFile(tName.to_string() + ".dds");

        es::string_view texData(buffer.data() + t.lowOffset, t.lowSize);
        SendTexelsLB(texData, ctx);
      }
    }
  }

  if (!lowTextures) {
    return;
  }

  for (auto &t : lowTextures->textures) {
    es::string_view tName(t.name.Get());
    if (processedTextures.contains(tName)) {
      continue;
    }

    ctx->NewFile(tName.to_string() + ".dds");

    es::string_view texData(reinterpret_cast<char *>(lowTextures) + t.lowOffset,
                            t.lowSize);
    SendTexelsLB(texData, ctx);
  }
}

template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

void TryExtactMDO(BinReaderRef rd, AppExtractContext *ctx) {
  auto mdoPath = ctx->ctx->workingFile;
  mdoPath.replace(mdoPath.size() - 3, 3, "mdo");
  MXMD::Wrap mxmd;
  {
    auto mdoStream = ctx->ctx->RequestFile(mdoPath);
    BinReaderRef mdo(*mdoStream.Get());
    using ex = MXMD::Wrap::ExcludeLoad;
    mxmd.Load(mdo, {}, {ex::Materials, ex::Model, ex::Shaders});
  }

  auto var = MXMD::GetVariantFromWrapper(mxmd);

  std::visit(overloaded{
                 [ctx, rd](MXMD::V1::Header &hdr) {
                   ExtractSimple(rd, hdr.uncachedTextures, hdr.cachedTextures,
                                 ctx, SendTexelsGTX);
                 },
                 [ctx, rd](MXMD::V2::Header &hdr) {
                   ExtractSimple(rd, hdr.uncachedTextures, hdr.cachedTextures,
                                 ctx, SendTexelsLB);
                 },
                 [ctx, rd](MXMD::V3::Header &hdr) {
                   auto smtHdr = std::get<MXMD::V3::SMTHeader *>(hdr.GetSMT());
                   ExtractSimple(rd, smtHdr, hdr.cachedTextures, ctx);
                 },
             },
             var);
}

void AppExtractFile(std::istream &stream, AppExtractContext *ctx) {
  BinReaderRef rd(stream);
  AFileInfo info(ctx->ctx->workingFile);

  if (info.GetExtension() == ".wismt" || info.GetExtension() == ".pcsmt") {
    try {
      TryExtractDRSM(rd, ctx);
    } catch (const es::InvalidHeaderError &e) {
      TryExtactMDO(rd, ctx);
    }
  } else {
    TryExtactMDO(rd, ctx);
  }
}

size_t AppExtractStat(request_chunk requester) {
  auto data = requester(0, 16);
  auto *hdr = reinterpret_cast<DRSM::Header *>(data.data());

  if (hdr->id == DRSM::ID) {
    auto resData = requester(16, reinterpret_cast<uint32 &>(hdr->streamData));
    auto *res = reinterpret_cast<DRSM::Resources *>(resData.data());
    ProcessClass(*res);

    if (res->textures) {
      return res->textures->textures.numItems;
    }
  }

  return 0;
}
