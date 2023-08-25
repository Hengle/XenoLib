/*  Xenoblade Engine Format Library
    Copyright(C) 2017-2023 Lukas Cone

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
#include "spike/util/supercore.hpp"
#include <string_view>

namespace LBIM {
static constexpr uint32 ID = CompileFourCC("LBIM");

enum class Format {
  BC1 = 66,
  BC2 = 67,
  BC3 = 68,
  BC4_UNORM = 73,
  BC5_UNORM = 75,
  BC7 = 77,
  BC6H_UF16 = 80,
  RGBA8888 = 37,
  RGBA4444 = 57,
  R8 = 1,
  R16G16B16A16 = 41,
  BGRA8888 = 109,
};

enum class Type {
  Texture2D = 1,
  Volumetric = 2,
  Cubemap = 8,
};

struct Header {
  uint32 datasize;
  uint32 headersize; // page size?
  uint32 width;
  uint32 height;
  uint32 depth;
  Type type;
  Format format;
  uint32 numMips;
  uint32 version;
  uint32 id;
};

const Header *Mount(std::string_view data) {
  return reinterpret_cast<const Header *>(&*data.end() - sizeof(Header));
}
} // namespace LBIM
