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
#include "datas/supercore.hpp"
#include "internal/gx2.hpp"
#include "xenolib/core.hpp"

namespace MTHS {
template <class C> using Pointer = es::PointerX86<C>;

template <class C> struct Array {
  uint32 numItems;
  Pointer<C> items;

  C *begin() { return items; }
  C *end() { return begin() + numItems; }

  void FixupRelative(const char *base) { items.FixupRelative(base); }
};

struct UniformBlock {
  uint32 unk0, unk1, unk2;
};

struct FragmentShader {
  Pointer<uint32> registries;
  uint32 numRegistries; // constant
  Array<uint8> program;
  gx2::ShaderMode shaderMode;
  Array<gx2::UniformBlock> uniformBlocks;
  Array<gx2::UniformValue> uniformVars;
  uint32 null01[4];
  Array<gx2::Sampler> samplers;
};

struct VertexShader : FragmentShader {
  Array<gx2::Attribute> attributes;
  uint32 null02[6];
};

static constexpr int ID = CompileFourCC("MTHS");

struct Header {
  uint32 id;
  uint32 version;
  Pointer<VertexShader> vertexShader;
  Pointer<FragmentShader> fragmentShader;
  Pointer<char> geometryShader;
  Pointer<char> uniformBlocks;
  Pointer<char> uniformVars;
  Pointer<char> attributes;
  Pointer<char> samplers;
  Pointer<char> registries;
  Pointer<char> strings;
  Pointer<char> programs;
};
} // namespace MTHS
