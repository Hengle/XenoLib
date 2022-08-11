/*  BDAT2JSON
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

#include "datas/app_context.hpp"
#include "datas/binreader_stream.hpp"
#include "datas/endian.hpp"
#include "datas/except.hpp"
#include "datas/fileinfo.hpp"
#include "datas/reflector.hpp"
#include "nlohmann/json.hpp"
#include "project.h"
#include "xenolib/bdat.hpp"

static struct BDAT2JSON : ReflectorBase<BDAT2JSON> {
  bool extract = true;
} settings;

REFLECT(CLASS(BDAT2JSON),
        MEMBER(extract, "E", ReflDesc{"Extract by data entry if possible."}), );

es::string_view filters[]{
    ".bdat$",
    {},
};

static AppInfo_s appInfo{
    AppInfo_s::CONTEXT_VERSION,
    AppMode_e::EXTRACT,
    ArchiveLoadType::FILTERED,
    BDAT2JSON_DESC " v" BDAT2JSON_VERSION ", " BDAT2JSON_COPYRIGHT "Lukas Cone",
    reinterpret_cast<ReflectorFriend *>(&settings),
    filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

namespace BDAT::V1 {
nlohmann::json ToJSON(const Header *hdr) {
  nlohmann::json jk;
  const char *values = hdr->keyValues;
  const KeyDesc *keyDescs = hdr->keyDescs;
  struct Flag {
    es::string_view name;
    uint16 index;
    uint16 value;
  };
  std::map<const KeyDesc *, std::vector<Flag>> flags;

  for (uint16 k = 0; k < hdr->numKeyDescs; k++) {
    auto &cDesc = keyDescs[k];
    const BaseTypeDesc *kDesc = cDesc.typeDesc;

    if (kDesc->baseType == BaseType::Flag) {
      auto &flagType = *static_cast<const FlagTypeDesc *>(kDesc);
      Flag flg;
      flg.name = cDesc.name.Get();
      flg.index = flagType.index;
      flg.value = flagType.value;

      const KeyDesc *key = flagType.belongsTo;

      if (flags.contains(key)) {
        flags.at(key).emplace_back(flg);
      } else {
        flags.emplace(key, std::vector<Flag>{flg});
      }
    }
  }

  for (uint16 b = 0; b < hdr->numKeyValues; b++) {
    const char *block = values + hdr->kvBlockStride * b;
    nlohmann::json kvBlock;

    for (uint16 k = 0; k < hdr->numKeyDescs; k++) {
      auto &cDesc = keyDescs[k];
      const BaseTypeDesc *kDesc = cDesc.typeDesc;

      auto SetFlags = [](auto &flags, const Value *bVal, DataType type) {
        uint32 data;
        switch (type) {
        case DataType::i8:
          data = bVal->asI8;
          break;
        case DataType::i16:
          data = bVal->asI16;
          break;
        case DataType::i32:
          data = bVal->asI32;
          break;
        case DataType::u8:
          data = bVal->asU8;
          break;
        case DataType::u16:
          data = bVal->asU16;
          break;
        case DataType::u32:
          data = bVal->asU32;
          break;
        default:
          throw std::runtime_error("Invalid flag type");
        }

        std::map<es::string_view, bool> flagDict;

        for (Flag &f : flags) {
          flagDict.emplace(f.name, data & f.value);
        }

        auto retVal = nlohmann::json{flagDict};

        return retVal.is_array() ? retVal.back() : retVal;
      };

      auto SetValue = [&](nlohmann::json &at, size_t offset = 0) {
        auto &valueType = *static_cast<const TypeDesc *>(kDesc);
        auto bVal =
            reinterpret_cast<const Value *>(block + valueType.offset + offset);

        if (flags.contains(&cDesc)) {
          at = SetFlags(flags.at(&cDesc), bVal, valueType.type);
          return;
        }

        switch (valueType.type) {
        case DataType::i8:
          at = bVal->asI8;
          break;
        case DataType::i16:
          at = bVal->asI16;
          break;
        case DataType::i32:
          at = bVal->asI32;
          break;
        case DataType::u8:
          at = bVal->asU8;
          break;
        case DataType::u16:
          at = bVal->asU16;
          break;
        case DataType::u32:
          at = bVal->asU32;
          break;
        case DataType::StringPtr:
          at = bVal->asString.Get();
          break;
        case DataType::Float:
          at = bVal->asFloat;
          break;

        default:
          break;
        }
      };

      switch (kDesc->baseType) {
      case BaseType::Default:
        SetValue(kvBlock[cDesc.name.Get()]);
      case BaseType::Flag:
      case BaseType::None:
        break;

      case BaseType::Array: {
        auto &jv = kvBlock[cDesc.name.Get()];
        auto &valueType = *static_cast<const ArrayTypeDesc *>(kDesc);
        size_t typeLen = BDAT::TypeSize(valueType.type);

        for (uint16 a = 0; a < valueType.numItems; a++) {
          nlohmann::json cjVal;
          SetValue(cjVal, a * typeLen);
          jv.emplace_back(cjVal);
        }
      }
      }
    }

    jk.emplace_back(std::move(kvBlock));
  }

  return jk;
}

void Extract(BinReaderRef rd, AppExtractContext *ctx) {
  {
    Collection col;
    rd.Push();
    rd.Read(col);

    if (col.numDatas > 0x10000) {
      FByteswapper(col);
      FByteswapper(col.datas);
    }

    rd.Seek(reinterpret_cast<uint32 &>(col.datas[0]));
    uint32 bdatId;
    rd.Read(bdatId);
    rd.Pop();

    if (bdatId != BDAT::ID) {
      throw es::InvalidHeaderError(bdatId);
    }
  }

  std::string buffer;
  rd.ReadContainer(buffer, rd.GetSize());
  Collection &col = reinterpret_cast<BDAT::V1::Collection &>(*buffer.data());
  ProcessClass(col);

  if (settings.extract && col.numDatas > 1) {
    for (auto &d : col) {
      const Header *hdr = d;
      ctx->NewFile(hdr->name.Get() + std::string(".json"));
      auto dumped = ToJSON(hdr).dump(2, ' ');
      ctx->SendData(dumped);
    }
  } else {
    AFileInfo finf(ctx->ctx->workingFile);
    ctx->NewFile(finf.GetFilename().to_string() + ".json");
    ctx->SendData("{\n");

    for (auto &d : col) {
      const Header *hdr = d;
      ctx->SendData("\"");
      ctx->SendData(hdr->name.Get());
      ctx->SendData("\": ");
      auto dumped = ToJSON(hdr).dump(2, ' ');
      ctx->SendData(dumped);
    }
  }
}
} // namespace BDAT::V1

namespace BDAT::V4 {
nlohmann::json ToJSON(const Header *hdr) {
  nlohmann::json jk;
  const char *values = hdr->values;
  const Descriptor *descs = hdr->descriptors;

  for (uint16 b = 0; b < hdr->numKeys; b++) {
    const char *block = values + hdr->kvBlockSize * b;
    size_t curOffset = 0;
    nlohmann::json kvBlock;

    for (uint16 k = 0; k < hdr->numDescs; k++) {
      auto &cDesc = descs[k];

      auto SetValue = [&](nlohmann::json &at, size_t offset = 0) {
        auto bVal = reinterpret_cast<const Value *>(block + curOffset + offset);

        switch (cDesc.type) {
        case DataType::i8:
          at = bVal->asI8;
          break;
        case DataType::i16:
          at = bVal->asI16;
          break;
        case DataType::i32:
          at = bVal->asI32;
          break;
        case DataType::u8:
          at = bVal->asU8;
          break;
        case DataType::u16:
        case DataType::Unk1:
          at = bVal->asU16;
          break;
        case DataType::u32:
        case DataType::Unk:
          at = bVal->asU32;
          break;
        case DataType::StringPtr:
          at = bVal->asString.Get();
          break;
        case DataType::Float:
          at = bVal->asFloat;
          break;
        case DataType::KeyHash: {
          char data[0x10]{};
          snprintf(data, sizeof(data), "%" PRIX32, bVal->asU32);
          at = data;
          break;
        }
        default:
          break;
        }
      };

      SetValue(kvBlock[k]);
      curOffset += BDAT::TypeSize(cDesc.type);
    }

    jk.emplace_back(std::move(kvBlock));
  }

  return jk;
}

void Extract(BinReaderRef rd, AppExtractContext *ctx) {
  std::string buffer;
  rd.ReadContainer(buffer, rd.GetSize());
  Collection &col = reinterpret_cast<Collection &>(*buffer.data());
  ProcessClass(col, {});

  if (settings.extract && col.numDatas > 1) {
    for (size_t cid = 0; auto &d : col) {
      const Header *hdr = d;
      ctx->NewFile(std::to_string(cid++) + std::string(".json"));
      auto dumped = ToJSON(hdr).dump(2, ' ');
      ctx->SendData(dumped);
    }
  } else {
    AFileInfo finf(ctx->ctx->workingFile);
    ctx->NewFile(finf.GetFilename().to_string() + ".json");
    ctx->SendData("{\n");

    for (auto &d : col) {
      const Header *hdr = d;
      ctx->SendData("\"");
      ctx->SendData(finf.GetFilename());
      ctx->SendData("\": ");
      auto dumped = ToJSON(hdr).dump(2, ' ');
      ctx->SendData(dumped);
    }
  }
}
} // namespace BDAT::V4

void AppExtractFile(std::istream &stream, AppExtractContext *ctx) {
  BinReaderRef rd(stream);

  uint32 id;
  rd.Push();
  rd.Read(id);
  rd.Pop();

  if (id == BDAT::ID) {
    BDAT::V4::Extract(rd, ctx);
  } else {
    BDAT::V1::Extract(rd, ctx);
  }
}

size_t AppExtractStat(request_chunk requester) {
  if (!settings.extract) {
    return 1;
  }
  auto data = requester(0, 16);

  BDAT::V4::HeaderBase *v4base =
      reinterpret_cast<BDAT::V4::HeaderBase *>(data.data());

  if (v4base->id == BDAT::ID) {
    if (v4base->type == BDAT::V4::Type::Collection) {
      return static_cast<BDAT::V4::Collection *>(v4base)->numDatas;
    }

    return 1;
  }

  BDAT::V1::Collection *col =
      reinterpret_cast<BDAT::V1::Collection *>(data.data());

  if (col->numDatas > 0x10000) {
    FByteswapper(col->numDatas);
  }

  return col->numDatas;
}
