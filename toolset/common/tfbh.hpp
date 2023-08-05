/*  gltf shared stream
    Copyright(C) 2022-2023 Lukas Cone

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
#define ES_COPYABLE_POINTER

#include "spike/type/pointer.hpp"
#include "spike/type/vectors.hpp"
#include <array>

namespace TFBH {

enum class VtType : uint8 {
  Position,
  Normal,
  Tangent,
  TexCoord,
  TexCoordUnsigned,
  Color,
};

enum class VtViewSlot : uint8 {
  Float3,
  Float2,
  Short2,
  Short4,
  Byte4,
  IndexShort,
  IndexInt,
  NumSlots,
};

uint32 GetStride(VtViewSlot slot) {
  switch (slot) {
  case VtViewSlot::Byte4:
  case VtViewSlot::Short2:
    return 4;
  case VtViewSlot::Float2:
  case VtViewSlot::Short4:
    return 8;
  case VtViewSlot::Float3:
    return 12;
  default:
    return 0;
  }
}

struct VtView {
  uint32 offset;
  uint32 size;
};

struct VtAttr {
  uint32 viewOffset;
  VtType type;
  VtViewSlot viewSlot;
};

struct VtIndex {
  uint32 viewOffset;
  uint32 count;
  VtViewSlot viewSlot;
};

template <class C> struct Array {
  uint32 numItems;
  es::PointerX86<C> items;

  const C *begin() const { return items; }
  const C *end() const { return begin() + numItems; }
  C *begin() { return items; }
  C *end() { return begin() + numItems; }
  const C &operator[](size_t index) const { return begin()[index]; }
  C &operator[](size_t index) { return begin()[index]; }
};

struct VertexBlock {
  Vector bboxMax;
  Vector bboxMin;
  uint32 numVertices;
  Array<VtAttr> attributes;
};

struct Stream {
  Array<VtIndex> indices;
  Array<VertexBlock> vertices;
  std::array<VtView, uint32(VtViewSlot::NumSlots)> views;
};

static constexpr uint32 ID = CompileFourCC("TFBH");

struct Header {
  uint32 id = ID;
  uint32 version = 1;
  uint32 bufferSize;
  Array<Stream> streams;
};

template <class C> struct Builder;

template <class I> class BuilderItem {
  std::reference_wrapper<std::string> base;
  size_t offset;

public:
  BuilderItem(std::string &builder, size_t offset_)
      : base{builder}, offset{offset_} {}

  operator I &() { return Get(); }
  I *operator->() { reinterpret_cast<I *>(base.get().data() + offset); }
  I &Get() { return *reinterpret_cast<I *>(base.get().data() + offset); }
};

template <class C> struct Builder {
  std::string buffer;

  Builder() {
    buffer.reserve(4096);
    buffer.resize(sizeof(C));
    Data() = {};
  }

  C &Data() { return *reinterpret_cast<C *>(buffer.data()); }

  template <class A>
  BuilderItem<Array<A>> SetArray(Array<A> &array, size_t numItems) {
    const size_t dataBegin = buffer.size();
    array.numItems = numItems;
    const size_t arrayOffset = reinterpret_cast<size_t>(&array) -
                               reinterpret_cast<size_t>(buffer.data());
    array.items.Reset(dataBegin - (arrayOffset + 4));

    buffer.resize(dataBegin + sizeof(A) * numItems);

    return {buffer, arrayOffset};
  }
};

} // namespace TFBH
