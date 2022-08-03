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

#include "xenolib/drsm.hpp"
#include "datas/endian.hpp"
#include "datas/except.hpp"
#include "xenolib/xbc1.hpp"
#include <cassert>

template <> void XN_EXTERN FByteswapper(DRSM::Textures &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(DRSM::Texture &item, bool) {
  FArraySwapper(item);
}

template <>
void XN_EXTERN ProcessClass(DRSM::Textures &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoAutoDetect();
  flags.base = reinterpret_cast<char *>(&item);

  if (flags == ProcessFlag::EnsureBigEndian) {
    FArraySwapper(item);
  }

  es::FixupPointers(flags.base, item.names, item.textures);

  for (auto &t : item.textures) {
    if (flags == ProcessFlag::EnsureBigEndian) {
      FByteswapper(t);
    }
    t.name.Fixup(flags.base);
  }
}

template <>
void XN_EXTERN ProcessClass(DRSM::Resources &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoBigEndian();

  if (flags == ProcessFlag::AutoDetectEndian) {
    flags -= ProcessFlag::AutoDetectEndian;
  }

  if (item.id != 0x1001) {
    throw es::InvalidHeaderError(item.id);
  }

  flags.base = reinterpret_cast<char *>(&item);

  es::FixupPointers(flags.base, item.externalResources, item.streamEntries,
                    item.streams, item.textureIndices, item.textures);
  if (item.textures) {
    ProcessClass(*item.textures, flags);
    assert(item.lowTexturesStreamEntryIndex == 2);
    assert(item.lowTexturesStreamIndex == 0);
    assert(item.middleTexturesStreamEntryBeginIndex == 3 ||
           item.middleTexturesStreamEntryBeginIndex == 0);
    assert(item.middleTexturesStreamIndex == 1 ||
           item.middleTexturesStreamEntryBeginIndex == 0);
  }
  assert(item.modelStreamEntryIndex == 0);
  assert(item.shaderStreamEntryIndex == 1);
}

template <>
void XN_EXTERN ProcessClass(DRSM::Header &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoBigEndian();

  if (flags == ProcessFlag::AutoDetectEndian) {
    flags -= ProcessFlag::AutoDetectEndian;
  }

  if (item.id != DRSM::ID) {
    throw es::InvalidHeaderError(item.id);
  }

  if (item.version != 0x2711) {
    throw es::InvalidVersionError(item.version);
  }

  flags.base = reinterpret_cast<char *>((&item)) + 1;

  item.streamData.Fixup(flags.base);

  flags.base = reinterpret_cast<char *>(&item);

  item.resources.Fixup(flags.base);

  ProcessClass(*item.resources, flags);

  for (auto &s : item.resources->streams) {
    s.data.Fixup(flags.base);
  }
}

std::string DRSM::Stream::GetData() const { return DecompressXBC1(data); }
