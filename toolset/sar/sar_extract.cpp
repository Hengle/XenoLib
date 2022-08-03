/*  SARExtract
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
#include "xenolib/sar.hpp"

es::string_view filters[]{
    ".sar$",
    ".chr$",
    ".mot$",
    ".arc$",
    {},
};

static AppInfo_s appInfo{
    AppInfo_s::CONTEXT_VERSION,
    AppMode_e::EXTRACT,
    ArchiveLoadType::FILTERED,
    SARExtract_DESC " v" SARExtract_VERSION ", " SARExtract_COPYRIGHT
                    "Lukas Cone",
    nullptr,
    filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

void AppExtractFile(std::istream &stream, AppExtractContext *ctx) {
  BinReaderRef rd(stream);
  AFileInfo info(ctx->ctx->workingFile);

  {
    SAR::Header hdr;
    rd.Push();
    rd.Read(hdr);
    rd.Pop();

    if (hdr.id != SAR::ID && hdr.id != SAR::ID_BIG) {
      throw es::InvalidHeaderError(hdr.id);
    }
  }

  std::string buffer;
  rd.ReadContainer(buffer, rd.GetSize());

  SAR::Header *hdr = reinterpret_cast<SAR::Header *>(buffer.data());
  ProcessClass(*hdr);

  for (auto &e : hdr->entries) {
    ctx->NewFile(e.fileName);
    ctx->SendData({e.data.Get(), e.dataSize});
  }
}

size_t AppExtractStat(request_chunk requester) {
  auto data = requester(0, sizeof(SAR::Header));
  auto *hdr = reinterpret_cast<SAR::Header *>(data.data());

  if (hdr->id == SAR::ID) {
    return hdr->entries.numItems;
  } else if (hdr->id == SAR::ID_BIG) {
    FByteswapper(*hdr);
    return hdr->entries.numItems;
  }

  return 0;
}
