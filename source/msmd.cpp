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

#include "xenolib/msmd.hpp"
#include "datas/endian.hpp"
#include "datas/except.hpp"

template <> void XN_EXTERN FByteswapper(MSMD::BBOX &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(MSMD::Bounds &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(MSMD::DataSpan &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(MSMD::V1::EmbededHKX &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(MSMD::V1::Grass &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(MSMD::V1::Header &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(MSMD::ObjectModel &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(MSMD::ObjectTextureFile &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(MSMD::SkyboxModel &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(MSMD::StreamEntry &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(MSMD::V1::TerrainLODModel &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(MSMD::TerrainModel &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(MSMD::V1::TGLDEntry &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(MSMD::HeaderBase &item, bool) {
  FArraySwapper(item);
}

template <>
void XN_EXTERN FByteswapper(MSMD::V1::TerrainTextureHeader &item, bool) {
  FArraySwapper(item);
}

template <>
void XN_EXTERN FByteswapper(MSMD::V1::TerrainTexture &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(MSMD::V1::SkyboxHeader &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(MSMD::V1::BVSCBlockEntry &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(MSMD::V1::BVSCBlock &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(MSMD::V1::LandmarkHeader &item, bool) {
  FArraySwapper(item);
}

template <>
void XN_EXTERN ProcessClass(MSMD::V1::TerrainTextureHeader &item,
                            ProcessFlags flags) {
  flags.NoAutoDetect();
  flags.NoLittleEndian();
  flags.NoProcessDataOut();
  FByteswapper(item);
  flags.base = reinterpret_cast<char *>(&item);
  item.textures.Fixup(flags.base);

  for (auto &t : item.textures) {
    FByteswapper(t);
  }
}

template <>
void XN_EXTERN ProcessClass(MSMD::V1::BVSCBlockEntry &item,
                            ProcessFlags flags) {
  flags.NoAutoDetect();
  flags.NoLittleEndian();
  flags.NoProcessDataOut();
  FByteswapper(item);
  item.data.Fixup(flags.base);
}

template <>
void XN_EXTERN ProcessClass(MSMD::V1::BVSCBlock &item, ProcessFlags flags) {
  flags.NoAutoDetect();
  flags.NoLittleEndian();
  flags.NoProcessDataOut();
  FByteswapper(item);
  item.entries.Fixup(flags.base);
  item.main.Fixup(flags.base);

  for (auto &e : item.entries) {
    ProcessClass(e, flags);
  }
}

template <>
void XN_EXTERN ProcessClass(MSMD::DataSpan &item, ProcessFlags flags) {
  flags.NoAutoDetect();
  flags.NoProcessDataOut();
  if (flags == ProcessFlag::EnsureBigEndian) {
    FByteswapper(item);
  }
  item.data.Fixup(flags.base);
}

template <>
void XN_EXTERN ProcessClass(MSMD::V1::Header &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoLittleEndian();
  flags.NoAutoDetect();
  FByteswapper(item);

  es::FixupPointers(
      flags.base, item.BVSC, item.cachedTGLD, item.CEMS, item.effects,
      item.grass, item.havokNames, item.hkColisions, item.LCMD,
      item.mapTerrainBuffers, item.objects, item.objectStreams,
      item.objectTextures, item.skyboxModels, item.terrainCachedTextures,
      item.terrainLODModels, item.terrainModels, item.terrainStreamingTextures,
      item.TGLDNames, item.TGLDs, item.unk00, item.unk01, item.unk02);

  for (auto &e : item.effects) {
    FByteswapper(e);
  }
  for (auto &e : item.grass) {
    FByteswapper(e);
  }
  for (auto &e : item.hkColisions) {
    FByteswapper(e);
  }
  for (auto &e : item.mapTerrainBuffers) {
    FByteswapper(e);
  }
  for (auto &e : item.objects) {
    FByteswapper(e);
  }
  for (auto &e : item.objectStreams) {
    FByteswapper(e);
  }
  for (auto &e : item.objectTextures) {
    FByteswapper(e);
  }
  for (auto &e : item.skyboxModels) {
    FByteswapper(e);
  }
  for (auto &e : item.terrainCachedTextures) {
    FByteswapper(e);
  }
  for (auto &e : item.terrainLODModels) {
    FByteswapper(e);
  }
  for (auto &e : item.terrainModels) {
    FByteswapper(e);
  }
  for (auto &e : item.terrainStreamingTextures) {
    FByteswapper(e);
  }
  for (auto &e : item.TGLDNames) {
    FByteswapper(e);
    e.Fixup(flags.base);
  }
  for (auto &e : item.TGLDs) {
    FByteswapper(e);
  }
  for (auto &e : item.unk00) {
    FByteswapper(e);
  }
  for (auto &e : item.unk01) {
    FByteswapper(e);
  }
  for (auto &e : item.unk02) {
    FByteswapper(e);
  }

  ProcessClass(*item.BVSC.items, flags);
}

template <>
void XN_EXTERN ProcessClass(MSMD::V2::Header &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoBigEndian();
  flags.NoAutoDetect();

  es::FixupPointers(flags.base, item.cachedTGLD, item.objects,
                    item.objectStreams, item.objectTextures, item.skyboxModels,
                    item.terrainCachedTextures, item.terrainModels, item.unk0,
                    item.nerd, item.lbigs, item.dlcm);
}

template <>
void XN_EXTERN ProcessClass(MSMD::HeaderBase &item, ProcessFlags flags) {
  flags.NoProcessDataOut();

  if (flags == ProcessFlag::AutoDetectEndian) {
    flags -= ProcessFlag::AutoDetectEndian;

    if (item.id == MSMD::ID_BIG) {
      flags += ProcessFlag::EnsureBigEndian;
    } else if (item.id == MSMD::ID) {
      flags -= ProcessFlag::EnsureBigEndian;
    } else {
      throw es::InvalidHeaderError(item.id);
    }
  }

  MSMD::Version version = item.version;

  if (flags == ProcessFlag::EnsureBigEndian) {
    FByteswapper(reinterpret_cast<uint32 &>(version));
  }

  flags.base = reinterpret_cast<char *>(&item);

  if (version == MSMD::Version::V10011) {
    ProcessClass(static_cast<MSMD::V1::Header &>(item), flags);
  } else if (version == MSMD::Version::V10112) {
    ProcessClass(static_cast<MSMD::V2::Header &>(item), flags);
  } else {
    throw es::InvalidVersionError(uint32(item.version));
  }
}
