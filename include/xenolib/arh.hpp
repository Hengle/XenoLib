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
#include "datas/supercore.hpp"

namespace ARH {
static constexpr uint32 ID = CompileFourCC("arh1");

struct FileEntry {
  uint64 dataOffset;
  uint32 compressedSize;
  uint32 uncompressedSize;
  uint32 compressed;
  uint32 index;
};

/* Tail Leaf
    char tailName[];
    uint32 fileId;
*/

/*
    if a < 0 and b > 0:
        a is -ptr to Tail leaf
        b is next node index in case a comp fails?
*/
struct ABNode {
  int32 a;
  int32 b;
};

struct Header {
  uint32 id;
  uint32 keySeed;
  uint32 numNodes;
  uint32 tailLeafsBuffer;
  uint32 tailLeafsBufferSize;
  uint32 trieBuffer;
  uint32 trieBufferSize;
  uint32 fileEntries;
  uint32 numFiles;
  uint32 unk;
};
} // namespace ARH
