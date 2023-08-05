/*  Xenoblade Engine Format Library
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
#include "core.hpp"
#include "spike/type/pointer.hpp"
#include "spike/util/supercore.hpp"
#include <string_view>
#include <stdexcept>

namespace BDAT {
constexpr static uint32 ID = CompileFourCC("BDAT");
template <class C> using Pointer = es::PointerX86<C>;

enum class DataType : uint8 {
  None,
  u8,
  u16,
  u32,
  i8,
  i16,
  i32,
  StringPtr,
  Float,
  KeyHash,
  Unk = 11,  // Pointer??
  Unk1 = 13, // Child index??
};

inline size_t TypeSize(DataType type) {
  switch (type) {
  case BDAT::DataType::i16:
  case BDAT::DataType::u16:
  case BDAT::DataType::Unk1:
    return 2;
  case BDAT::DataType::i32:
  case BDAT::DataType::u32:
  case BDAT::DataType::StringPtr:
  case BDAT::DataType::Float:
  case BDAT::DataType::KeyHash:
  case BDAT::DataType::Unk:
    return 4;
  case BDAT::DataType::i8:
  case BDAT::DataType::u8:
    return 1;
  default:
    throw std::runtime_error("Unhandled data type");
  }
}

union Value {
  int8 asI8;
  int16 asI16;
  int32 asI32;
  uint8 asU8;
  uint16 asU16;
  uint32 asU32;
  float asFloat;
  Pointer<char> asString;
};

#include "internal/bdat1.inl"
#include "internal/bdat4.inl"
} // namespace BDAT
