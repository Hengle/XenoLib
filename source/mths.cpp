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

#include "xenolib/mths.hpp"
#include "datas/endian.hpp"
#include "datas/except.hpp"
#include <cassert>

template <> void XN_EXTERN FByteswapper(MTHS::VertexShader &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(MTHS::FragmentShader &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(MTHS::Header &item, bool) {
  FArraySwapper(item);
}

template <>
void XN_EXTERN ProcessClass(MTHS::FragmentShader &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoLittleEndian();
  flags.NoAutoDetect();

  FByteswapper(item);

  MTHS::Header *hdr = reinterpret_cast<MTHS::Header *>(flags.base);
  item.program.FixupRelative(hdr->programs);
  item.registries.FixupRelative(hdr->registries);
  item.samplers.FixupRelative(hdr->samplers);
  item.uniformBlocks.FixupRelative(hdr->uniformBlocks);
  item.uniformVars.FixupRelative(hdr->uniformVars);

  [hdr](auto &...item) {
    (
        [hdr](auto &item) {
          for (auto &a : item) {
            FArraySwapper(a);
            a.name.FixupRelative(hdr->strings);
          }
        }(item),
        ...);
  }(item.samplers, item.uniformBlocks, item.uniformVars);

  assert(item.null01[0] == 0);
  assert(item.null01[1] == 0);
  assert(item.null01[2] == 0);
  assert(item.null01[3] == 0);
}

template <>
void XN_EXTERN ProcessClass(MTHS::VertexShader &item, ProcessFlags flags) {
  flags.NoProcessDataOut();
  flags.NoLittleEndian();
  flags.NoAutoDetect();

  ProcessClass(static_cast<MTHS::FragmentShader &>(item), flags);
  FArraySwapper(item.attributes);

  MTHS::Header *hdr = reinterpret_cast<MTHS::Header *>(flags.base);
  item.attributes.FixupRelative(hdr->attributes);

  for (auto &a : item.attributes) {
    FArraySwapper(a);
    a.name.FixupRelative(hdr->strings);
  }

  assert(item.null02[0] == 0);
  assert(item.null02[1] == 0);
  assert(item.null02[2] == 0);
  assert(item.null02[3] == 0);
  assert(item.null02[4] == 0);
  assert(item.null02[5] == 0);
}

template <>
void XN_EXTERN ProcessClass(MTHS::Header &item, ProcessFlags flags) {
  flags.NoProcessDataOut();

  if (flags == ProcessFlag::AutoDetectEndian) {
    flags -= ProcessFlag::AutoDetectEndian;

    if (item.id == MTHS::ID) {
      flags += ProcessFlag::EnsureBigEndian;
    } else {
      throw es::InvalidHeaderError(item.id);
    }
  }

  if (item.id != MTHS::ID) {
    throw es::InvalidHeaderError(item.id);
  }

  flags.NoLittleEndian();
  FByteswapper(item);

  if (item.version != 0x2711) {
    throw es::InvalidVersionError(item.version);
  }

  flags.base = reinterpret_cast<char *>(&item);

  es::FixupPointers(flags.base, item.attributes, item.geometryShader,
                    item.fragmentShader, item.programs, item.registries,
                    item.samplers, item.strings, item.uniformBlocks,
                    item.uniformVars, item.vertexShader);

  if (item.vertexShader) {
    ProcessClass(*item.vertexShader.Get(), flags);
  }

  if (item.fragmentShader) {
    ProcessClass(*item.fragmentShader.Get(), flags);
  }
}
