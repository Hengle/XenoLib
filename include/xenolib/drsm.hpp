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
#include "datas/pointer.hpp"
#include "datas/string_view.hpp"
#include "datas/supercore.hpp"
#include "xenolib/core.hpp"

namespace DRSM {
template <class C> struct Array {
  uint32 numItems;
  es::PointerX86<C> items;

  C *begin() { return items; }
  C *end() { return begin() + numItems; }

  void Fixup(const char *base) { items.Fixup(base); }
};

enum ResourceEntryType : uint16 {
  Model,
  Shaders,
  LowTextures,
  MiddleTextures,
};

struct StreamEntry {
  uint32 offset;
  uint32 size;
  uint16 unkIndex;
  ResourceEntryType entryType;
  uint32 null[2];
};

struct Stream {
  uint32 compressedSize;
  uint32 uncompressedSize;
  es::PointerX86<char> data;

  std::string XN_EXTERN GetData() const;
};

struct Texture {
  uint32 unk;
  uint32 lowSize;
  uint32 lowOffset;
  es::PointerX86<char> name;
};

struct Textures {
  Array<Texture> textures;
  uint32 null;
  es::PointerX86<char> names;
};

struct ExternalResource {
  uint32 hash;
  uint32 mediumUncompressedSize;
  uint32 unk0;
  uint32 highUncompressedSize;
  uint32 unk1;
};

struct Resources {
  uint32 id; // 0x1001
  uint32 version;
  Array<StreamEntry> streamEntries;
  Array<Stream> streams;
  uint32 modelStreamEntryIndex;
  uint32 shaderStreamEntryIndex;
  uint32 lowTexturesStreamEntryIndex;
  uint32 lowTexturesStreamIndex;
  uint32 middleTexturesStreamIndex;
  uint32 middleTexturesStreamEntryBeginIndex;
  uint32 numMiddleTexturesStreamEntries;
  Array<int16> textureIndices;
  es::PointerX86<Textures> textures;
  uint32 null0;
  Array<ExternalResource> externalResources;
};

static constexpr uint32 ID = CompileFourCC("DRSM");

struct Header {
  uint32 id;
  uint32 version;
  es::PointerX86<char> streamData;
  es::PointerX86<Resources> resources;
};
} // namespace DRSM
