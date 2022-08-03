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
#include "datas/pointer.hpp"

namespace SAR {
static constexpr uint32 ID_BIG = CompileFourCC("SAR1");
static constexpr uint32 ID = CompileFourCC("1RAS");

template <class C> struct Array {
  uint32 numItems;
  es::PointerX86<C> items;

  C *begin() { return items; }
  C *end() { return begin() + numItems; }

  void Fixup(const char *base) { items.Fixup(base); }
};

struct FileEntry {
  es::PointerX86<char> data;
  uint32 dataSize;
  uint32 nameHash;
  char fileName[52];

  void XN_EXTERN RecalcHash();
};

struct Header {
  uint32 id;
  uint32 fileSize;
  uint32 version;
  Array<FileEntry> entries;
  es::PointerX86<char> data;
  uint32 unk0;
  uint32 unk1;
  char mainPath[128];
};
} // namespace SAR
