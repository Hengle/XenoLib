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
#include "project.h"
#include "xenolib/sar.hpp"

std::string_view filters[]{".sar$", ".chr$", ".mot$", ".arc$"};

static AppInfo_s appInfo{
    .filteredLoad = true,
    .header = SARExtract_DESC " v" SARExtract_VERSION ", " SARExtract_COPYRIGHT
                              "Lukas Cone",
    .filters = filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

void AppProcessFile(AppContext *ctx) {
  {
    SAR::Header hdr;
    ctx->GetType(hdr);

    if (hdr.id != SAR::ID && hdr.id != SAR::ID_BIG) {
      throw es::InvalidHeaderError(hdr.id);
    }
  }

  auto buffer = ctx->GetBuffer();
  SAR::Header *hdr = reinterpret_cast<SAR::Header *>(buffer.data());
  ProcessClass(*hdr);
  auto ectx = ctx->ExtractContext();

  for (auto &e : hdr->entries) {
    ectx->NewFile(e.fileName);
    ectx->SendData({e.data.Get(), e.dataSize});
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
