/*  DecARH
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

#include "project.h"
#include "spike/app_context.hpp"
#include "spike/except.hpp"
#include "spike/io/binreader_stream.hpp"
#include "spike/io/binwritter_stream.hpp"
#include "xenolib/arh.hpp"

std::string_view filters[]{
    ".arh$",
};

static AppInfo_s appInfo{
    .filteredLoad = true,
    .header =
        DecARH_DESC " v" DecARH_VERSION ", " DecARH_COPYRIGHT "Lukas Cone",
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

  BinWritterRef wr(
      ctx->NewFile(ctx->workingFile.ChangeExtension2("arhdec")).str);
  wr.Write(hdr);

  rd.Seek(hdr.tailLeafsBuffer);
  uint32 key;
  rd.Read(key);
  key ^= hdr.keySeed;
  std::vector<uint32> tailBuffer;
  rd.ReadContainer(tailBuffer, (hdr.tailLeafsBufferSize / 4) - 1);

  for (auto &d : tailBuffer) {
    d ^= key;
  }
  hdr.tailLeafsBuffer = wr.Tell();
  hdr.tailLeafsBufferSize = 0;
  wr.Write(key);
  wr.WriteContainer(tailBuffer);

  std::vector<ARH::ABNode> trieBuffer;
  rd.Seek(hdr.trieBuffer);
  rd.ReadContainer(trieBuffer, hdr.trieBufferSize / 8);

  for (auto &d : trieBuffer) {
    d.a ^= key;
    d.b ^= key;
  }
  hdr.trieBuffer = wr.Tell();
  hdr.trieBufferSize = 0;
  wr.WriteContainer(trieBuffer);

  rd.Seek(hdr.fileEntries);
  std::vector<ARH::FileEntry> entries;
  rd.ReadContainer(entries, hdr.numFiles);
  hdr.fileEntries = wr.Tell();
  wr.WriteContainer(entries);
  wr.Seek(0);
  wr.Write(hdr);

  auto &wrds = ctx->NewFile(ctx->workingFile.ChangeExtension2("arhdump")).str;
  auto *tb = reinterpret_cast<const char *>(tailBuffer.data());

  for (size_t index = 0; auto t : trieBuffer) {
    if (t.a < 0 && t.b > 0) {
      std::string_view tail = tb - t.a - 4;
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

      wrds << curPath << tail << '\n';
    }

    index++;
  }
}
