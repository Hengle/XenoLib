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

#include "xenolib/bdat.hpp"
#include "datas/app_context.hpp"
#include "datas/binreader_stream.hpp"
#include "datas/endian.hpp"
#include "datas/except.hpp"
#include "datas/fileinfo.hpp"
#include "datas/reflector.hpp"
#include "nlohmann/json.hpp"
#include "project.h"

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

nlohmann::json ToJSON(const BDAT::Header *hdr) {
  nlohmann::json jk;
  const char *values = hdr->keyValues;
  const BDAT::KeyDesc *keyDescs = hdr->keyDescs;

  for (uint16 b = 0; b < hdr->numKeyValues; b++) {
    const char *block = values + hdr->kvBlockStride * b;
    nlohmann::json kvBlock;

    for (uint16 k = 0; k < hdr->numKeyDescs; k++) {
      auto &cDesc = keyDescs[k];
      const BDAT::BaseTypeDesc *kDesc = cDesc.typeDesc;

      auto SetValue = [&](nlohmann::json &at, size_t offset = 0) {
        auto &valueType = *static_cast<const BDAT::TypeDesc *>(kDesc);
        auto bVal = reinterpret_cast<const BDAT::Value *>(
            block + valueType.offset + offset);

        switch (valueType.type) {
        case BDAT::DataType::i8:
          at = bVal->asI8;
          break;
        case BDAT::DataType::i16:
          at = bVal->asI16;
          break;
        case BDAT::DataType::i32:
          at = bVal->asI32;
          break;
        case BDAT::DataType::u8:
          at = bVal->asU8;
          break;
        case BDAT::DataType::u16:
          at = bVal->asU16;
          break;
        case BDAT::DataType::u32:
          at = bVal->asU32;
          break;
        case BDAT::DataType::StringPtr:
          at = bVal->asString.Get();
          break;
        case BDAT::DataType::Float: {
          // Encryption is not enough?
          // We also have to have encrypted floats?
          constexpr uint64 coec = 0x4330000080000000;
          uint64 raw = coec ^ bVal->asU32;
          double dbl = reinterpret_cast<double &>(raw) -
                       reinterpret_cast<const double &>(coec);
          at = dbl * (1.f / 4096);
          break;
        }
        default:
          break;
        }
      };

      switch (kDesc->baseType) {
      case BDAT::BaseType::Default:
        SetValue(kvBlock[cDesc.name.Get()]);
      case BDAT::BaseType::Flag:
      case BDAT::BaseType::None:
        break;

      case BDAT::BaseType::Array: {
        auto &jv = kvBlock[cDesc.name.Get()];
        auto &valueType = *static_cast<const BDAT::ArrayTypeDesc *>(kDesc);
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

void AppExtractFile(std::istream &stream, AppExtractContext *ctx) {
  BinReaderRef rd(stream);
  {
    BDAT::Collection col;
    rd.Push();
    rd.Read(col);

    if (col.numDatas > 0x10000) {
      FByteswapper(col);
      FByteswapper(col.datas);
    } else {
      throw std::runtime_error("Invalid format.");
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
  BDAT::Collection &col = reinterpret_cast<BDAT::Collection &>(*buffer.data());
  ProcessClass(col, {ProcessFlag::EnsureBigEndian});

  if (settings.extract && col.numDatas > 1) {
    for (auto &d : col) {
      const BDAT::Header *hdr = d;
      ctx->NewFile(hdr->name.Get() + std::string(".json"));
      auto dumped = ToJSON(hdr).dump(2, ' ');
      ctx->SendData(dumped);
    }
  } else {
    AFileInfo finf(ctx->ctx->workingFile);
    ctx->NewFile(finf.GetFilename().to_string() + ".json");
    ctx->SendData("{\n");

    for (auto &d : col) {
      const BDAT::Header *hdr = d;
      ctx->SendData("\"");
      ctx->SendData(hdr->name.Get());
      ctx->SendData("\": ");
      auto dumped = ToJSON(hdr).dump(2, ' ');
      ctx->SendData(dumped);
    }
  }
}

size_t AppExtractStat(request_chunk requester) {
  if (!settings.extract) {
    return 1;
  }
  auto data = requester(0, 8);
  BDAT::Collection *col = reinterpret_cast<BDAT::Collection *>(data.data());

  if (col->numDatas > 0x10000) {
    FByteswapper(col->numDatas);
  }

  return col->numDatas;
}
