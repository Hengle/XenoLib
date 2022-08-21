/*  Xenoblade Engine Format Library
    Copyright(C) 2019-2022 Lukas Cone

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

namespace BC {
static constexpr uint32 ID = CompileFourCC("BC\0\0");

template <class C> struct Array {
  C *items;
  uint32 numItems;
  uint32 unk;

  C *begin() { return items; }
  C *end() { return begin() + numItems; }
};

struct Block {
  uint32 unk;
  uint32 id;
};

struct Header {
  uint32 id;
  uint16 unk0;
  uint16 numBlocks;
  uint32 fileSize;
  uint32 numPointers;
  Block *data;
  uint64 *fixups;
};
} // namespace BC
