/*  DecBDAT
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
#include "datas/binwritter_stream.hpp"
#include "datas/endian.hpp"
#include "datas/except.hpp"
#include "project.h"
#include "xenolib/bdat.hpp"

std::string_view filters[]{
    ".bdat$",
};

static AppInfo_s appInfo{
    .filteredLoad = true,
    .header =
        DecBDAT_DESC " v" DecBDAT_VERSION ", " DecBDAT_COPYRIGHT "Lukas Cone",
    .filters = filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

namespace BDAT::V1 {
struct HeaderImpl : Header {
  void XN_EXTERN DecryptSection(char *, char *);
};
} // namespace BDAT::V1

void AppProcessFile(std::istream &stream, AppContext *ctx) {
  BinReaderRef rd(stream);
  bool endianBig = false;
  {
    BDAT::V1::Collection col;
    rd.Push();
    rd.Read(col);

    if (col.numDatas > 0x10000) {
      FByteswapper(col);
      FByteswapper(col.datas);
      endianBig = true;
    }

    rd.Seek(reinterpret_cast<uint32 &>(col.datas[0]));
    uint32 bdatId;
    rd.Read(bdatId);
    rd.Pop();

    if (bdatId != BDAT::ID) {
      throw es::InvalidHeaderError(bdatId);
    }
  }

  std::string buffer;
  rd.ReadContainer(buffer, rd.GetSize());
  BDAT::V1::Collection &col =
      reinterpret_cast<BDAT::V1::Collection &>(*buffer.data());
  if (endianBig) {
    FByteswapper(col);
  }
  char *base = reinterpret_cast<char *>(&col);

  for (auto &p : col) {
    if (endianBig) {
      FByteswapper(p);
    }
    p.Fixup(base);
    BDAT::V1::Header *hdr = p;
    if (endianBig) {
      FByteswapper(*hdr);
    }
    char *hbase = reinterpret_cast<char *>(hdr);
    [&](auto &...item) {
      (item.Fixup(hbase), ...);
    }(hdr->name, hdr->unk1Offset, hdr->keyValues, hdr->strings, hdr->keyDescs);

    static_cast<BDAT::V1::HeaderImpl *>(hdr)->DecryptSection(
        hdr->name.Get(), hdr->unk1Offset.Get());

    if (char *strings = hdr->strings; hdr->stringsSize) {
      static_cast<BDAT::V1::HeaderImpl *>(hdr)->DecryptSection(
          strings, strings + hdr->stringsSize);
    }
  }

  BinWritterRef wr(ctx->NewFile(ctx->workingFile.ChangeExtension(".bdatdec")));
  wr.WriteContainer(buffer);
}
