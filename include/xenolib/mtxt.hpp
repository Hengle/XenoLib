/*  Xenoblade Engine Format Library
    Copyright(C) 2017-2022 Lukas Cone

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

#pragma once
#include "core.hpp"
#include "datas/string_view.hpp"
#include "datas/supercore.hpp"
#include "internal/gx2.hpp"

namespace MTXT {
static constexpr uint32 ID = CompileFourCC("MTXT");

struct Header {
  uint32 swizzle;
  gx2::SurfaceDim dimension;
  uint32 width;
  uint32 height;
  uint32 depth;
  uint32 numMips;
  gx2::SurfaceFormat type;
  uint32 size;
  gx2::AAMode AAMode;
  gx2::TileMode tilingMode;
  uint32 unk;
  uint32 alignment;
  uint32 pitch;
  uint32 null[13];
  uint32 version;
  uint32 id;
};

const Header *Mount(es::string_view data) {
  return reinterpret_cast<const Header *>(data.end() - sizeof(Header));
}

void XN_EXTERN DecodeMipmap(const Header &header, const char *data,
                            char *outData, uint32 mipIndex = 0);

} // namespace MTXT
