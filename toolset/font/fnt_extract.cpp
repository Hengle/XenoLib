/*  FNTExtract
    Copyright(C) 2023 Lukas Cone

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

#include "nlohmann/json.hpp"
#include "project.h"
#include "spike/app_context.hpp"
#include "spike/except.hpp"
#include "spike/io/binreader_stream.hpp"
#include "spike/io/binwritter_stream.hpp"
#include "spike/master_printer.hpp"
#include "spike/util/unicode.hpp"
#include "texture.hpp"

std::string_view filters[]{
    ".fnt$",
};

static AppInfo_s appInfo{
    .filteredLoad = true,
    .header = FNTExtract_DESC " v" FNTExtract_VERSION ", " FNTExtract_COPYRIGHT
                              "Lukas Cone",
    .filters = filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

struct Char {
  uint16 character;
  uint16 unkChracter;
  uint8 charWidth;
  uint8 charBegin;
};

template <> void FByteswapper(Char &item, bool) { FArraySwapper<uint16>(item); }

struct FNTHeader {
  uint32 byteSize; // 2 = utf-16
  uint32 fileSize;
  uint32 unk1; // 0x10
  uint32 textureOffset;
  uint32 gridWidth;
  uint32 gridHeight;
  uint32 numChars;
  uint32 unk2[4];
  uint32 numColumns;
  uint32 numRows;
};

template <> void FByteswapper(FNTHeader &item, bool) { FArraySwapper(item); }

void AppProcessFile(AppContext *ctx) {
  std::string fntData = ctx->GetBuffer();
  FNTHeader *fntHeader = reinterpret_cast<FNTHeader *>(fntData.data());

  FByteswapper(*fntHeader);

  if (fntHeader->byteSize != 2) {
    throw std::runtime_error("Unknown font byte size");
  }

  auto hdr = MTXT::Mount(fntData);

  {
    if (hdr->id != MTXT::ID) {
      throw es::InvalidHeaderError(hdr->id);
    }
    MTXTAddrLib arrdLib;
    FByteswapper(*const_cast<MTXT::Header *>(hdr));
    ctx->NewImage(
        MakeContext(hdr, arrdLib, fntData.data() + fntHeader->textureOffset));
  }

  std::span<Char> chars(reinterpret_cast<Char *>(fntHeader + 1),
                        fntHeader->numChars);

  nlohmann::json main;
  auto &grid = main["grid"];
  grid["width"] = fntHeader->gridWidth;
  grid["height"] = fntHeader->gridHeight;
  grid["rows"] = fntHeader->numRows;
  grid["columns"] = fntHeader->numColumns;
  auto &rows = main["rows"] = nlohmann::json::array();

  size_t curCharId = 0;

  for (auto c : chars) {
    FByteswapper(c);

    if (curCharId % fntHeader->numRows == 0) {
      rows.emplace_back(nlohmann::json::array());
    }

    auto &curRow = rows.back();
    auto &curChar = curRow.emplace_back();
    uint16 chars[2]{c.character, 0};
    curChar["character"] = es::ToUTF8(chars);
    chars[0] = c.unkChracter;
    curChar["unkChracter"] = es::ToUTF8(chars);
    curChar["offset"] = c.charBegin;
    curChar["width"] = c.charWidth;

    curCharId++;
  }

  ctx->NewFile(ctx->workingFile.ChangeExtension2("json")).str << std::setw(2)
                                                              << main;
}
