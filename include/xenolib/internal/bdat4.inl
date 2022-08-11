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

namespace V4 {
using namespace BDAT;

enum class Type : uint8 {
  Data,
  Collection,
};

struct HeaderBase {
  uint32 id;
  uint8 version;
  uint8 headerSize;
  uint8 null;
  Type type;
};

struct Descriptor {
  DataType type;
  uint8 nameOffset0;
  uint8 nameOffset1;
  uint16 NameOffset() const { return nameOffset0 + (nameOffset1 * 256); }
};

struct Key {
  uint32 hash;
  uint32 index;
};

struct Header : HeaderBase {
  uint32 numDescs;
  uint32 numKeys;
  uint32 unk3;
  uint32 selfHash; // could be used as encKey?
  Pointer<Descriptor> descriptors;
  Pointer<Key> keys;
  Pointer<char> values;
  uint32 kvBlockSize;
  Pointer<char> strings;
  uint32 stringsSize;
};

struct Collection : HeaderBase {
  uint32 numDatas;
  uint32 fileSize;
  Pointer<Header> datas[1];

  Pointer<Header> *begin() { return datas; }
  Pointer<Header> *end() { return datas + numDatas; }
  const Pointer<Header> *begin() const { return datas; }
  const Pointer<Header> *end() const { return datas + numDatas; }
};

} // namespace V4
