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

#include "xenolib/bdat.hpp"
#include "datas/endian.hpp"
#include "datas/except.hpp"
#include <cassert>
#include <random>

namespace BDAT::V1 {
bool KVPair::operator==(const Value &other) const {
  if (desc->baseType != BaseType::Default) {
    return false; // throw?
  }

  auto &asDef = *static_cast<const TypeDesc *>(desc);

  switch (asDef.type) {
  case DataType::i8:
  case DataType::u8:
    return other.asU8 == value->asU8;

  case DataType::i16:
  case DataType::u16:
    return other.asU16 == value->asU16;

  case DataType::i32:
  case DataType::u32:
    return other.asU16 == value->asU16;

  case DataType::Float:
    return other.asFloat == value->asFloat;

  case DataType::StringPtr:
    throw std::runtime_error("Invalid call for string comparison");
  default:
    return false;
  }
}

struct HeaderImpl : Header {
  void XN_EXTERN DecryptSection(char *begin, char *end) {
    uint8 curKey[]{uint8(~encKeys[1]), uint8(~encKeys[0])};
    size_t curIndex = 0;
    while (begin < end) {
      uint8 c = *begin;
      *begin ^= curKey[curIndex % 2];
      curKey[curIndex % 2] += c;
      curIndex++;
      begin++;
    }
  }

  void XN_EXTERN EncryptSection(char *begin, char *end) {
    std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<uint32> distrib;
    uint32 rndKey = distrib(gen);
    encKeys[0] = rndKey;
    encKeys[1] = rndKey >> 16;
    uint8 curKey[]{uint8(~encKeys[1]), uint8(~encKeys[0])};
    size_t curIndex = 0;
    while (begin < end) {
      *begin ^= curKey[curIndex % 2];
      curKey[curIndex % 2] += *begin;
      curIndex++;
      begin++;
    }
  }

  const KeyDesc *FindKey(es::string_view name) {
    KeyDesc *ck = keyDescs;
    for (uint16 k = 0; k < numKeyDescs; k++) {
      if (name == ck[k].name.Get()) {
        return ck;
      }
    }

    return nullptr;
  }

  template <class C> const char *FindBlock_(es::string_view keyName, C &value) {
    auto key = FindKey(keyName);

    if (!key) {
      return nullptr;
    }

    KVPair pair;
    pair.desc = key->typeDesc;

    const char *values = keyValues;
    for (uint16 b = 0; b < numKeyValues; b++) {
      pair.data = values + kvBlockStride * b;

      if (pair == value) {
        return values;
      }
    }

    return nullptr;
  }
};

const char *Header::FindBlock(es::string_view keyName, Value value) {
  return static_cast<HeaderImpl *>(this)->FindBlock_(keyName, value);
}

const char *Header::FindBlock(es::string_view keyName, es::string_view value) {
  return static_cast<HeaderImpl *>(this)->FindBlock_(keyName, value);
}

const Header *Collection::FindData(es::string_view name) const {
  for (auto &h : *this) {
    if (name == h->name.Get()) {
      return h;
    }
  }

  return nullptr;
}
} // namespace BDAT::V1

template <> void XN_EXTERN FByteswapper(BDAT::V1::FlagTypeDesc &item, bool) {
  FByteswapper(item.belongsTo);
  FByteswapper(item.null);
  FByteswapper(item.value);
  assert(item.null == 0);
}

template <> void XN_EXTERN FByteswapper(BDAT::V1::TypeDesc &item, bool) {
  FByteswapper(item.offset);
}

template <> void XN_EXTERN FByteswapper(BDAT::V1::ArrayTypeDesc &item, bool) {
  FByteswapper(item.numItems);
  FByteswapper(item.offset);
}

template <> void XN_EXTERN FByteswapper(BDAT::V1::KeyDesc &item, bool) {
  FByteswapper(item.name);
  FByteswapper(item.unk);
  FByteswapper(item.typeDesc);
}

template <> void XN_EXTERN FByteswapper(BDAT::V1::Header &item, bool) {
  [](auto &...item) {
    (FByteswapper(item), ...);
  }(item.name, item.kvBlockStride, item.unk1Offset, item.unk1Size,
    item.keyValues, item.numKeyValues, item.unk3, item.numEncKeys, item.strings,
    item.stringsSize, item.keyDescs, item.numKeyDescs);
  std::swap(item.encKeys[0], item.encKeys[1]);
}

template <> void XN_EXTERN FByteswapper(BDAT::V1::Collection &item, bool) {
  FByteswapper(item.numDatas);
  FByteswapper(item.fileSize);
}

template <>
void XN_EXTERN ProcessClass(BDAT::V1::FlagTypeDesc &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoAutoDetect();

  if (flags == ProcessFlag::EnsureBigEndian) {
    FByteswapper(item);
  }
  item.belongsTo.Fixup(flags.base);
}

template <>
void XN_EXTERN ProcessClass(BDAT::V1::BaseTypeDesc &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoAutoDetect();

  switch (item.baseType) {
  case BDAT::V1::BaseType::Default:
    if (flags == ProcessFlag::EnsureBigEndian) {
      FByteswapper(static_cast<BDAT::V1::TypeDesc &>(item));
    }
    break;
  case BDAT::V1::BaseType::Array:
    if (flags == ProcessFlag::EnsureBigEndian) {
      FByteswapper(static_cast<BDAT::V1::ArrayTypeDesc &>(item));
    }
    break;
  case BDAT::V1::BaseType::Flag:
    ProcessClass(static_cast<BDAT::V1::FlagTypeDesc &>(item), flags);
    break;
  default:
    throw std::runtime_error("Undefined base type");
  }
}

template <>
void XN_EXTERN ProcessClass(BDAT::V1::KeyDesc &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoAutoDetect();

  if (flags == ProcessFlag::EnsureBigEndian) {
    FByteswapper(item);
  }

  item.name.Fixup(flags.base);
  item.unk.Fixup(flags.base);
  item.typeDesc.Fixup(flags.base);

  ProcessClass(*item.typeDesc.Get(), flags);
}

template <>
void XN_EXTERN ProcessClass(BDAT::V1::Header &item, ProcessFlags flags) {
  if (item.id != BDAT::ID) {
    throw es::InvalidHeaderError(item.id);
  }

  flags.NoProcessDataOut();
  flags.NoAutoDetect();

  if (flags == ProcessFlag::EnsureBigEndian) {
    FByteswapper(item);
  }

  flags.base = reinterpret_cast<char *>(&item);
  assert(item.numEncKeys == 2);

  [&](auto &...item) {
    (item.Fixup(flags.base), ...);
  }(item.name, item.unk1Offset, item.keyValues, item.strings, item.keyDescs);

  static_cast<BDAT::V1::HeaderImpl &>(item).DecryptSection(
      item.name.Get(), item.unk1Offset.Get());

  if (char *strings = item.strings; item.stringsSize) {
    static_cast<BDAT::V1::HeaderImpl &>(item).DecryptSection(
        strings, strings + item.stringsSize);
  }

  BDAT::V1::KeyDesc *ck = item.keyDescs;
  for (uint16 k = 0; k < item.numKeyDescs; k++) {
    ProcessClass(ck[k], flags);
  }

  char *values = item.keyValues;
  const BDAT::V1::KeyDesc *keyDescs = item.keyDescs;

  for (uint16 k = 0; k < item.numKeyValues; k++) {
    char *block = values + item.kvBlockStride * k;

    for (uint16 k = 0; k < item.numKeyDescs; k++) {
      auto &cDesc = keyDescs[k];
      const BDAT::V1::BaseTypeDesc *kDesc = cDesc.typeDesc;

      auto SwapValue = [&](size_t itemOffset = 0) {
        auto &valueType = *static_cast<const BDAT::V1::TypeDesc *>(kDesc);
        auto bVal = reinterpret_cast<BDAT::Value *>(block + valueType.offset +
                                                    itemOffset);
        switch (valueType.type) {
        case BDAT::DataType::i16:
        case BDAT::DataType::u16:
          if (flags == ProcessFlag::EnsureBigEndian) {
            FByteswapper(bVal->asU16);
          }
          break;
        case BDAT::DataType::i32:
        case BDAT::DataType::u32:
          if (flags == ProcessFlag::EnsureBigEndian) {
            FByteswapper(bVal->asU32);
          }
          break;
        case BDAT::DataType::Float:
          if (flags == ProcessFlag::EnsureBigEndian) {
            FByteswapper(bVal->asU32);
          }

          if (item.flags == BDAT::V1::Type::EncryptFloat) {
            constexpr uint64 coec = 0x4330000080000000;
            uint64 raw = coec ^ bVal->asU32;
            double dbl = reinterpret_cast<double &>(raw) -
                         reinterpret_cast<const double &>(coec);
            bVal->asFloat = dbl * (1.f / 4096);
          }
          break;
        case BDAT::DataType::StringPtr:
          if (flags == ProcessFlag::EnsureBigEndian) {
            FByteswapper(bVal->asU32);
          }
          bVal->asString.Fixup(flags.base);
          break;
        case BDAT::DataType::i8:
        case BDAT::DataType::u8:
          break;
        default:
          throw std::runtime_error("Unhandled data type");
        }
      };

      switch (kDesc->baseType) {
      case BDAT::V1::BaseType::Default:
        SwapValue();
      case BDAT::V1::BaseType::Flag:
        break;
      case BDAT::V1::BaseType::Array: {
        auto &valueType = *static_cast<const BDAT::V1::ArrayTypeDesc *>(kDesc);
        size_t typeLen = [&] {
          switch (valueType.type) {
          case BDAT::DataType::i16:
          case BDAT::DataType::u16:
            return 2;
            break;
          case BDAT::DataType::i32:
          case BDAT::DataType::u32:
          case BDAT::DataType::StringPtr:
          case BDAT::DataType::Float:
            return 4;
            break;
          case BDAT::DataType::i8:
          case BDAT::DataType::u8:
            return 1;
            break;
          default:
            throw std::runtime_error("Unhandled data type");
          }
        }();

        for (uint16 a = 0; a < valueType.numItems; a++) {
          SwapValue(a * typeLen);
        }
        break;
      }

      break;
      default:
        throw std::runtime_error("Unhandled base data type");
      }
    }
  }
}

template <>
void XN_EXTERN ProcessClass(BDAT::V1::Collection &item, ProcessFlags flags) {
  flags.NoProcessDataOut();

  if (flags == ProcessFlag::AutoDetectEndian) {
    flags -= ProcessFlag::AutoDetectEndian;

    if (item.numDatas > 0x10000) {
      flags += ProcessFlag::EnsureBigEndian;
    }
  }

  flags.base = reinterpret_cast<char *>(&item);

  if (flags == ProcessFlag::EnsureBigEndian) {
    FByteswapper(item);
  }

  for (auto &p : item) {
    if (flags == ProcessFlag::EnsureBigEndian) {
      FByteswapper(p);
    }
    p.Fixup(flags.base);
    ProcessClass(*p.Get(), flags);
  }
}

template <>
void XN_EXTERN ProcessClass(BDAT::V4::Header &item, ProcessFlags flags) {
  if (item.id != BDAT::ID) {
    throw es::InvalidHeaderError(item.id);
  }

  flags.NoProcessDataOut();
  flags.NoAutoDetect();
  flags.NoBigEndian();

  flags.base = reinterpret_cast<char *>(&item);

  [&](auto &...item) {
    (item.Fixup(flags.base), ...);
  }(item.descriptors, item.keys, item.values, item.strings);

  char *values = item.values;
  const BDAT::V4::Descriptor *keyDescs = item.descriptors;
  size_t curOffset = 0;

  for (uint16 k = 0; k < item.numDescs; k++) {
    auto &cDesc = keyDescs[k];

    if (cDesc.type == BDAT::DataType::StringPtr) {
      for (uint16 k = 0; k < item.numKeys; k++) {
        char *block = values + item.kvBlockSize * k;
        auto str = reinterpret_cast<BDAT::Pointer<char> *>(block + curOffset);
        str->FixupRelative(item.strings);
      }
    }

    curOffset += BDAT::TypeSize(cDesc.type);
  }
}

template <>
void XN_EXTERN ProcessClass(BDAT::V4::Collection &item, ProcessFlags flags) {
  if (item.id != BDAT::ID) {
    throw es::InvalidHeaderError(item.id);
  }

  flags.NoProcessDataOut();
  flags.NoBigEndian();
  flags.NoAutoDetect();

  if (item.type != BDAT::V4::Type::Collection) {
    throw std::runtime_error("Supplied data are not collection");
  }

  flags.base = reinterpret_cast<char *>(&item);

  for (auto &p : item) {
    p.Fixup(flags.base);
    ProcessClass(*p.Get(), flags);
  }
}
