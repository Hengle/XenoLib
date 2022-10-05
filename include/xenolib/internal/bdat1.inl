/*  Xenoblade Engine Format Library
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

#pragma once

namespace V1 {
using namespace BDAT;
template <class C> using Pointer16 = es::PointerX86<C, int16>;

enum class BaseType : uint8 {
  None,
  Default,
  Array,
  Flag,
};

struct KeyDesc;

struct BaseTypeDesc {
  BaseType baseType;
};

struct FlagTypeDesc : BaseTypeDesc {
  uint8 index;
  uint16 null;
  uint16 value;
  Pointer16<KeyDesc> belongsTo;
};

struct TypeDesc : BaseTypeDesc {
  DataType type;
  uint16 offset;
};

struct ArrayTypeDesc : TypeDesc {
  uint16 numItems;
};

struct KeyDesc {
  Pointer16<BaseTypeDesc> typeDesc;
  Pointer16<KeyDesc> unk; // random refs
  Pointer16<char> name;
};

struct KVPair {
  const BaseTypeDesc *desc;
  union {
    const char *data;
    const Value *value;
  };

  bool XN_EXTERN operator==(const Value &other) const;

  bool operator==(std::string_view other) const {
    if (IsString()) {
      return other == value->asString.Get();
    }

    throw std::runtime_error("Invalid call for string comparison");
  }

  bool IsString() const {
    if (desc->baseType != BaseType::Default) {
      return false;
    }

    auto &asDef = *static_cast<const TypeDesc *>(desc);

    return asDef.type == DataType::StringPtr;
  }
};

enum class Type : uint8 {
  EncryptFloat,
  PersistentFlag, // Is this whole encryption?
};

using Flags = es::Flags<Type>;

struct Header {
  uint32 id;
  uint8 version;
  Flags flags;
  Pointer16<char> name;
  uint16 kvBlockStride;
  Pointer16<char> unk1Offset;
  uint16 unk1Size;
  Pointer16<char> keyValues;
  uint16 numKeyValues;
  uint16 unk3;
  uint16 numEncKeys;
  uint8 encKeys[2];
  Pointer<char> strings;
  uint32 stringsSize;
  Pointer16<KeyDesc> keyDescs;
  uint16 numKeyDescs;

  const char XN_EXTERN *FindBlock(std::string_view keyName, Value value);
  const char XN_EXTERN *FindBlock(std::string_view keyName,
                                  std::string_view value);
};

struct Collection {
  uint32 numDatas;
  uint32 fileSize;
  Pointer<Header> datas[1];

  Pointer<Header> *begin() { return datas; }
  Pointer<Header> *end() { return datas + numDatas; }
  const Pointer<Header> *begin() const { return datas; }
  const Pointer<Header> *end() const { return datas + numDatas; }

  const Header XN_EXTERN *FindData(std::string_view name) const;
};
} // namespace V1
