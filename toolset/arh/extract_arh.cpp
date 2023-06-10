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
#include "datas/master_printer.hpp"
#include "project.h"
#include "xenolib/arh.hpp"
#include "xenolib/xbc1.hpp"

std::string_view filters[]{
    ".arh$",
};

static AppInfo_s appInfo{
    .filteredLoad = true,
    .multithreaded = false,
    .header = ARHExtract_DESC " v" ARHExtract_VERSION ", " ARHExtract_COPYRIGHT
                              "Lukas Cone",
    .filters = filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

void AppProcessFile(AppContext *ctx) {
  BinReaderRef rd(ctx->GetStream());
  ARH::Header hdr;
  rd.Read(hdr);

  if (hdr.id != ARH::ID) {
    throw es::InvalidHeaderError(hdr.id);
  }

  auto dataStream = ctx->RequestFile(
      std::string(ctx->workingFile.GetFullPathNoExt()) + ".ard");
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
      std::string_view tail = tb - t.a - 4;
      uint32 fileIndex;
      memcpy(&fileIndex, &*tail.end() + 1, 4);

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
      fileNames[fileIndex] = curPath + std::string(tail);
    }

    index++;
  }

  es::Dispose(tailBuffer);
  es::Dispose(trieBuffer);
  auto ectx = ctx->ExtractContext();

  if (ectx->RequiresFolders()) {
    for (auto &f : fileNames) {
      AFileInfo file(f);
      ectx->AddFolderPath(std::string(file.GetFolder()));
    }

    ectx->GenerateFolders();
  }

  rd.Seek(hdr.fileEntries);
  std::string dataBuffer;

  for (size_t f = 0; f < hdr.numFiles; f++) {
    ARH::FileEntry entry;
    rd.Read(entry);
    auto &fileName = fileNames.at(entry.index);

    if (fileName.empty()) {
      printwarning("Skipped empty filename id: " << entry.index);
      continue;
    }

    ectx->NewFile(fileName);

    dataRd.Seek(entry.dataOffset);

    if (entry.compressed) {
      dataRd.ReadContainer(dataBuffer, entry.compressedSize + 64);
      auto data = DecompressXBC1(dataBuffer.data());
      ectx->SendData(data);
    } else {
      dataRd.ReadContainer(dataBuffer, entry.compressedSize);
      ectx->SendData(dataBuffer);
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
