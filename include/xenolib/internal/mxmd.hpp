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
#include "datas/vectors.hpp"
#include "uni/model.hpp"
#include "xenolib/drsm.hpp"
#include <variant>

namespace MXMD {
template <class C> using Pointer = es::PointerX86<C>;
using String = Pointer<char>;

template <class C> struct Array {
  Pointer<C> items;
  uint32 numItems;

  C *begin() { return items; }
  C *end() { return begin() + numItems; }

  void Fixup(const char *base) { items.Fixup(base); }
};

template <class C> struct ArrayInv {
  uint32 numItems;
  Pointer<C> items;

  C *begin() { return items; }
  C *end() { return begin() + numItems; }

  void Fixup(const char *base) { items.Fixup(base); }
};

template <es::IsFixable... C>
void FixupAndProcess(ProcessFlags flags, C &...item) {
  (
      [&] {
        if (item.Fixup(flags.base) > 0) {
          ProcessClass(*item, flags);
        }
      }(),
      ...);
}

enum Versions {
  MXMDVer1 = 10040,
  MXMDVer2 = 10111,
  MXMDVer3 = 10112,
};

struct HeaderBase {
  uint32 magic, version;
};

struct TransformMatrix {
  Vector4 m[4];
};

enum class VertexDescriptorType : uint16 {
  POSITION,
  WEIGHT32,
  BONEID,
  WEIGHTID,
  VERTEXCOLOR2,
  UV1,
  UV2,
  UV3,
  UV4,
  VERTEXCOLOR3 = 14,
  NORMAL32,
  TANGENT16,
  VERTEXCOLOR,
  NORMAL = 28,
  TANGENT,
  TANGENT2 = 31,
  NORMAL2,
  REFLECTION,
  WEIGHT16 = 41,
  BONEID2,
};

struct VertexType {
  VertexDescriptorType type;
  uint16 size;

  XN_EXTERN operator uni::FormatDescr() const;
  XN_EXTERN operator uni::PrimitiveDescriptor::Usage_e() const;
};

static constexpr int ID_BIG = CompileFourCC("MXMD");
static constexpr int ID = CompileFourCC("DMXM");

#include "mxmd_v1.inl"
#include "mxmd_v2.inl"
#include "mxmd_v3.inl"

using Variant = std::variant<std::reference_wrapper<V1::Header>,
                             std::reference_wrapper<V2::Header>,
                             std::reference_wrapper<V3::Header>>;
class Wrap;
Variant XN_EXTERN GetVariantFromWrapper(Wrap &wp);
} // namespace MXMD
