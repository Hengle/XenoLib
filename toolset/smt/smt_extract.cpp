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
#include "dds.hpp"
#include "project.h"
#include "xenolib/drsm.hpp"
#include "xenolib/internal/mxmd.hpp"
#include "xenolib/mxmd.hpp"
#include "xenolib/xbc1.hpp"
#include <set>

std::string_view filters[]{
    ".wismt$",
    ".casmt$",
    ".pcsmt$",
};

static constexpr bool DEV_EXTRACT_MODEL = false;
static constexpr bool DEV_EXTRACT_ALL = false;

static AppInfo_s appInfo{
    .filteredLoad = true,
    .header = SMTExtract_DESC " v" SMTExtract_VERSION ", " SMTExtract_COPYRIGHT
                              "Lukas Cone",
    .filters = filters,
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

void TryExtractDRSM(AppContext *ctx) {
  {
    DRSM::Header hdr;
    ctx->GetType(hdr);

    if (hdr.id != DRSM::ID) {
      throw es::InvalidHeaderError(hdr.id);
    }
  }

  std::string buffer = ctx->GetBuffer();

  DRSM::Header *hdr = reinterpret_cast<DRSM::Header *>(buffer.data());
  ProcessClass(*hdr);
  DRSM::Resources *resources = hdr->resources;
  DRSM::Textures *textures = resources->textures;
  DRSM::Stream *streams = resources->streams.items;
  DRSM::StreamEntry *entries = resources->streamEntries.items;
  LocalCache cache;
  auto ectx = ctx->ExtractContext();

  if constexpr (DEV_EXTRACT_ALL) {
    for (size_t index = 0; auto &s : resources->streams) {
      ectx->NewFile(std::to_string(index++));
      auto buffer = s.GetData();
      ectx->SendData(buffer);
    }

    return;
  }

  if constexpr (DEV_EXTRACT_MODEL) {
    const char *main = cache.GetSet(*streams, 0);

    {
      ectx->NewFile("vertexindexbuffer");
      auto &entry = entries[resources->modelStreamEntryIndex];
      std::string_view viStream(main + entry.offset, entry.size);
      ectx->SendData(viStream);
    }

    {
      ectx->NewFile("shaders.wishp");
      auto &entry = entries[resources->shaderStreamEntryIndex];
      std::string_view shStream(main + entry.offset, entry.size);
      ectx->SendData(shStream);
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

      std::string_view miMip(middleTextures + entry.offset, entry.size);
      auto tex = LBIM::Mount(miMip);
      ectx->NewFile(t.name.Get() + std::string(".dds"));
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
        SendDDS(&texCopy, ectx);
        LBIM::DecodeMipmap(texCopy, hiMip.data(), hiMipOut.data());
        ectx->SendData(hiMipOut);
      } else {
        std::string miMipOut;
        miMipOut.resize(miMip.size());
        SendDDS(tex, ectx);
        LBIM::DecodeMipmap(*tex, miMip.data(), miMipOut.data());
        ectx->SendData(miMipOut);
      }

    } else {
      numProcessedTextures++;
      ectx->NewFile(t.name.Get() + std::string(".dds"));
      auto &entry = entries[resources->lowTexturesStreamEntryIndex];
      auto loMips = cache.GetSet(streams[resources->lowTexturesStreamIndex], 0);
      std::string_view loMip(loMips + entry.offset + t.lowOffset, t.lowSize);

      if (loMip.starts_with("DDS")) {
        ectx->SendData(loMip);
      } else {
        std::string loMipOut;
        loMipOut.resize(loMip.size());
        auto tex = LBIM::Mount(loMip);
        SendDDS(tex, ectx);
        LBIM::DecodeMipmap(*tex, loMip.data(), loMipOut.data());
        ectx->SendData(loMipOut);
      }
    }
  }

  if (!numProcessedTextures) {
    throw std::runtime_error("No files.");
  }
}

void ExtractSimple(BinReaderRef rd, MXMD::V1::SMTHeader *hdr,
                   DRSM::Textures *cachedTextures, AppContext *ctx,
                   void (*dataSender)(std::string_view texData,
                                      AppExtractContext *ctx)) {
  std::set<std::string_view> processedTextures;
  auto ectx = ctx->ExtractContext();

  if (hdr) {
    for (int32 g = hdr->numUsedGroups - 1; g >= 0; g--) {
      auto textures = hdr->groups[g].Get();
      std::string buffer;
      rd.Seek(hdr->dataOffsets[g]);
      rd.ReadContainer(buffer, hdr->dataSizes[g]);

      for (auto &t : textures->textures) {
        std::string_view tName(t.name.Get());
        if (processedTextures.contains(tName)) {
          continue;
        }

        processedTextures.emplace(tName);
        ectx->NewFile(std::string(tName) + ".dds");
        std::string_view texData(buffer.data() + t.lowOffset, t.lowSize);
        dataSender(texData, ectx);
      }
    }
  }

  if (!cachedTextures) {
    return;
  }

  for (auto &t : cachedTextures->textures) {
    std::string_view tName(t.name.Get());
    if (processedTextures.contains(tName)) {
      continue;
    }

    ectx->NewFile(std::string(tName) + ".dds");

    std::string_view texData(
        reinterpret_cast<char *>(cachedTextures) + t.lowOffset, t.lowSize);
    dataSender(texData, ectx);
  }
}

void ExtractSimple(BinReaderRef rd, MXMD::V3::SMTHeader *hdr,
                   DRSM::Textures *lowTextures, AppContext *ctx) {
  std::set<std::string_view> processedTextures;
  auto ectx = ctx->ExtractContext();

  if (hdr) {
    for (int32 g = hdr->numUsedGroups - 1; g >= 0; g--) {
      auto textures = hdr->groups[g].Get();
      std::string buffer = [rd, g, hdr] {
        std::string buffer;
        rd.Seek(hdr->dataOffsets[g]);
        rd.ReadContainer(buffer, hdr->compressedSize[g]);
        return DecompressXBC1(buffer.data());
      }();

      for (auto &t : textures->textures) {
        std::string_view tName(t.name.Get());
        if (processedTextures.contains(tName)) {
          continue;
        }

        processedTextures.emplace(tName);
        ectx->NewFile(std::string(tName) + ".dds");

        std::string_view texData(buffer.data() + t.lowOffset, t.lowSize);
        SendTexelsLB(texData, ectx);
      }
    }
  }

  if (!lowTextures) {
    return;
  }

  for (auto &t : lowTextures->textures) {
    std::string_view tName(t.name.Get());
    if (processedTextures.contains(tName)) {
      continue;
    }

    ectx->NewFile(std::string(tName) + ".dds");

    std::string_view texData(
        reinterpret_cast<char *>(lowTextures) + t.lowOffset, t.lowSize);
    SendTexelsLB(texData, ectx);
  }
}

template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

void TryExtactMDO(AppContext *ctx) {
  std::string mdoPath(ctx->workingFile.GetFullPath());
  mdoPath.replace(mdoPath.size() - 3, 3, "mdo");
  MXMD::Wrap mxmd;
  {
    auto mdoStream = ctx->RequestFile(mdoPath);
    BinReaderRef mdo(*mdoStream.Get());
    using ex = MXMD::Wrap::ExcludeLoad;
    mxmd.Load(mdo, {}, {ex::Materials, ex::Model, ex::Shaders});
  }

  auto var = MXMD::GetVariantFromWrapper(mxmd);
  BinReaderRef rd(ctx->GetStream());

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

void AppProcessFile(AppContext *ctx) {

  if (ctx->workingFile.GetExtension() == ".wismt" ||
      ctx->workingFile.GetExtension() == ".pcsmt") {
    try {
      TryExtractDRSM(ctx);
    } catch (const es::InvalidHeaderError &e) {
      TryExtactMDO(ctx);
    }
  } else {
    TryExtactMDO(ctx);
  }
}
