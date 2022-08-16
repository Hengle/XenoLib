/*  DecARH
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
#include "datas/except.hpp"
#include "datas/fileinfo.hpp"
#include "project.h"
#include "xenolib/arh.hpp"

es::string_view filters[]{
    ".arh$",
    {},
};

static AppInfo_s appInfo{
    AppInfo_s::CONTEXT_VERSION,
    AppMode_e::CONVERT,
    ArchiveLoadType::FILTERED,
    DecARH_DESC " v" DecARH_VERSION ", " DecARH_COPYRIGHT "Lukas Cone",
    nullptr,
    filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

void AppProcessFile(std::istream &stream, AppContext *ctx) {
  BinReaderRef rd(stream);
  ARH::Header hdr;
  rd.Read(hdr);

  if (hdr.id != ARH::ID) {
    throw es::InvalidHeaderError(hdr.id);
  }

  AFileInfo outPath(ctx->outFile);
  BinWritter wr(outPath.GetFullPathNoExt().to_string() + ".arhdec");
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

  BinWritter_t<BinCoreOpenMode::Text> wrd(
      outPath.GetFullPathNoExt().to_string() + ".arhdump");
  auto &wrds = wrd.BaseStream();
  auto *tb = reinterpret_cast<const char *>(tailBuffer.data());

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

      wrds << curPath << tail << '\n';
    }

    index++;
  }

  rd.Seek(hdr.fileEntries);
  std::vector<ARH::FileEntry> entries;
  rd.ReadContainer(entries, hdr.numFiles);
  hdr.fileEntries = wr.Tell();
  wr.WriteContainer(entries);
  wr.Seek(0);
  wr.Write(hdr);
}
