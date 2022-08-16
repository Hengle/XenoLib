/*  ARHExtract
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
#include "project.h"
#include "xenolib/arh.hpp"
#include "xenolib/xbc1.hpp"

es::string_view filters[]{
    ".arh$",
    {},
};

static AppInfo_s appInfo{
    AppInfo_s::CONTEXT_VERSION,
    AppMode_e::EXTRACT,
    ArchiveLoadType::FILTERED,
    ARHExtract_DESC " v" ARHExtract_VERSION ", " ARHExtract_COPYRIGHT
                    "Lukas Cone",
    nullptr,
    filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

void AppExtractFile(std::istream &stream, AppExtractContext *ctx) {
  BinReaderRef rd(stream);
  ARH::Header hdr;
  rd.Read(hdr);

  if (hdr.id != ARH::ID) {
    throw es::InvalidHeaderError(hdr.id);
  }

  AFileInfo outPath(ctx->ctx->workingFile);
  auto dataStream =
      ctx->ctx->RequestFile(outPath.GetFullPathNoExt().to_string() + ".ard");
  BinReaderRef dataRd(*dataStream.Get());

  rd.Seek(hdr.tailLeafsBuffer);
  uint32 key;
  rd.Read(key);
  key ^= hdr.keySeed;
  std::vector<uint32> tailBuffer;
  rd.ReadContainer(tailBuffer, (hdr.tailLeafsBufferSize / 4) - 1);

  for (auto &d : tailBuffer) {
    d ^= key;
  }

  std::vector<ARH::ABNode> trieBuffer;
  rd.Seek(hdr.trieBuffer);
  rd.ReadContainer(trieBuffer, hdr.trieBufferSize / 8);

  for (auto &d : trieBuffer) {
    d.a ^= key;
    d.b ^= key;
  }

  auto *tb = reinterpret_cast<const char *>(tailBuffer.data());
  std::vector<std::string> fileNames;
  fileNames.resize(hdr.numFiles);

  for (size_t index = 0; auto t : trieBuffer) {
    if (t.a < 0 && t.b > 0) {
      es::string_view tail = tb - t.a - 4;
      uint32 fileIndex;
      memcpy(&fileIndex, tail.end() + 1, 4);

      int32 parentId = t.b;
      int32 curentXor = index;
      std::string curPath;
      while (parentId) {
        auto parent = trieBuffer[parentId];
        curPath.push_back(parent.a ^ curentXor);
        curentXor = parentId;
        parentId = parent.b;
      }

      std::reverse(curPath.begin(), curPath.end());
      fileNames[fileIndex] = curPath + tail.to_string();
    }

    index++;
  }

  es::Dispose(tailBuffer);
  es::Dispose(trieBuffer);

  if (ctx->RequiresFolders()) {
    for (auto &f : fileNames) {
      ctx->AddFolderPath(f);
    }

    ctx->GenerateFolders();
  }

  rd.Seek(hdr.fileEntries);
  std::string dataBuffer;

  for (size_t f = 0; f < hdr.numFiles; f++) {
    ARH::FileEntry entry;
    rd.Read(entry);
    ctx->NewFile(fileNames.at(entry.index));

    dataRd.Seek(entry.dataOffset);

    if (entry.compressed) {
      dataRd.ReadContainer(dataBuffer, entry.compressedSize + 64);
      auto data = DecompressXBC1(dataBuffer.data());
      ctx->SendData(data);
    } else {
      dataRd.ReadContainer(dataBuffer, entry.compressedSize);
      ctx->SendData(dataBuffer);
    }
  }
}

size_t AppExtractStat(request_chunk requester) {
  auto data = requester(0, sizeof(ARH::Header));
  auto *hdr = reinterpret_cast<ARH::Header *>(data.data());

  if (hdr->id == ARH::ID) {
    return hdr->numFiles;
  }

  return 0;
}
