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

#include "xenolib/lbim.hpp"
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <string>

void LBIM::DecodeMipmap(const Header &header, const char *data, char *outData,
                        uint32 mipIndex) {
  assert(mipIndex == 0); // not implemented
  uint32 bpp = 0;        // bytes per block/pixel
  uint32 ppb = 1;        // pixels per block

  switch (header.format) {
  case LBIM::Format::BC1:
  case LBIM::Format::BC4_UNORM:
    bpp = 8;
    ppb = 4;
    break;
  case LBIM::Format::BC2:
  case LBIM::Format::BC3:
  case LBIM::Format::BC5_UNORM:
  case LBIM::Format::BC7:
  case LBIM::Format::BC6H_UF16:
    bpp = 16;
    ppb = 4;
    break;
  case LBIM::Format::RGBA8888:
  case LBIM::Format::BGRA8888:
    bpp = 4;
    break;
  case LBIM::Format::RGBA4444:
    bpp = 2;
    break;
  case LBIM::Format::R8:
    bpp = 1;
    break;
  case LBIM::Format::R16G16B16A16:
    bpp = 8;
    break;
  default:
    throw std::runtime_error("Unhandled LBIM texture format: " +
                             std::to_string(uint32(header.format)));
  }

  auto NumLeadingZeroes = [](uint32 value) {
    int numZeros = 0;
    for (; (((value >> numZeros) & 1) == 0); numZeros++)
      ;
    return numZeros;
  };

  auto FindLastBit = [](uint32 value) {
    uint32 lastBit = 0;
    uint32 index = 0;

    while (value) {
      if (value & 1) {
        lastBit = 1 << index;
      }

      value >>= 1;
      index++;
    }

    return lastBit;
  };

  auto FindLastBitIndex = [](uint32 value) {
    uint32 index = 0;

    while (value) {
      value >>= 1;
      index++;
    }

    return index;
  };

  auto RoundUpPowTwo = [](size_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
  };

  const uint32 bWidth = header.width / ppb;
  const uint32 bHeight = header.height / ppb;
  const uint32 surfaceSize = bWidth * bHeight * bpp;
  const uint32 alignedSurfaceSize = bWidth * RoundUpPowTwo(bHeight) * bpp;

  const uint32 bppShift = NumLeadingZeroes(bpp);
  const uint32 lineShift = FindLastBitIndex(bWidth * bpp) - 1;

  uint32 xBitsShift = 3;
  const uint32 hBitMask = RoundUpPowTwo(bHeight) - 1;
  for (uint32 i = 3; i < 7; i++) {
    if (hBitMask & (1 << i)) {
      xBitsShift++;
    }
  }

  const size_t numSlices =
      header.type == Type::Cubemap ? 6 : std::max(header.depth, 1U);

  for (size_t s = 0; s < numSlices; s++) {
    auto outData_ = outData + surfaceSize * s;
    auto inData_ = data + alignedSurfaceSize * s;

    for (uint32 w = 0; w < bWidth; w++)
      for (uint32 h = 0; h < bHeight; h++) {

        // 12 11 10 9  8  7  6  5  4  3  2  1  0
        // y  y  y  y  x  y  y  x  y  x  x  x  x
        // y*x*y{0,4}xyyxyx[x|0]{4}

        constexpr uint32 yBitsTail = 0b1111'1111'1000'0000;
        constexpr uint32 yBits9_12 = 0b0000'0000'0111'1000;
        constexpr uint32 yBits6__7 = 0b0000'0000'0000'0110;
        constexpr uint32 yBitsHead = 0b0000'0000'0000'0001;

        constexpr uint32 xBitsTail = 0b1111'1111'1100'0000;
        constexpr uint32 xBitsEigh = 0b0000'0000'0010'0000;
        constexpr uint32 xBitsFith = 0b0000'0000'0001'0000;
        constexpr uint32 xBits0__3 = 0b0000'0000'0000'1111;

        const uint32 _X = w << bppShift;
        uint32 address = (h & yBitsTail) << lineShift // bits xEnd - yEnd
                         | ((h & yBits9_12) << 6)     // bits 9 - 12
                         | ((h & yBits6__7) << 5)     // bits 6,7
                         | ((h & yBitsHead) << 4)     // bit 4

                         | ((_X & xBitsTail) << xBitsShift) // bits 13 - xEnd
                         | ((_X & xBitsEigh) << 3)          // bit 8
                         | ((_X & xBitsFith) << 1)          // bit 5
                         | (_X & xBits0__3)                 // bits 0-3
            ;

        if (address + bpp > alignedSurfaceSize)
          continue;

        memcpy(outData_ + ((h * bWidth + w) * bpp), inData_ + address, bpp);
      }
  }
}
