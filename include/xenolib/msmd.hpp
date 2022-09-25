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

#include "core.hpp"
#include "datas/pointer.hpp"

namespace MSMD {
static constexpr uint32 ID_BIG = CompileFourCC("MSMD");
static constexpr uint32 ID = CompileFourCC("DMSM");

template <class C> using Pointer = es::PointerX86<C>;

template <class C> struct Array {
  uint32 numItems;
  Pointer<C> items;

  C *begin() { return items; }
  C *end() { return begin() + numItems; }

  void Fixup(const char *base) { items.Fixup(base); }
};

template <class C> struct ArrayInv {
  Pointer<C> items;
  uint32 numItems;

  C *begin() { return items; }
  C *end() { return begin() + numItems; }

  void Fixup(const char *base) { items.Fixup(base); }
};

struct DataSpan {
  Pointer<char> data;
  uint32 size;

  void Fixup(const char *base) { data.Fixup(base); }
};

enum class Version {
  V10011 = 10011,
  V10112 = 10112,
};

struct HeaderBase {
  uint32 id;
  Version version;
  uint32 null00[4];
};

struct StreamEntry {
  uint32 offset;
  uint32 size;
};

struct BBOX {
  float min[3];
  float max[3];
};

struct Bounds {
  BBOX bbox;
  float center[3];
};

struct TerrainModel {
  Bounds bounds;
  float unk03[4];
  StreamEntry entry;
  float unk04[4];
};

struct ObjectModel {
  Bounds bounds;
  float unk03[4];
  StreamEntry entry;
  uint32 unk01;
};

struct SkyboxModel {
  Bounds bounds;
  float unk03[4];
  StreamEntry entry;
};

struct ObjectTextureFile {
  StreamEntry midMap;
  StreamEntry highMap;
  uint32 unk;
};

namespace V1 {
struct EmbededHKX {
  Bounds bounds;
  float unk03[4];
  StreamEntry entry;
  uint32 unk00[3];
  uint32 nameOffset;
  uint32 unk01[3];
};

struct SkyboxHeader {
  uint32 models;
  uint32 materials;
  uint32 unk00;
  uint32 streams;
  uint32 cachedTextures;
  uint32 null00;
  uint32 shaders;
  uint32 null01[9];
};

struct LandmarkHeader {
  uint32 null[2];
  uint32 models;
  uint32 materials;
  uint32 unk00;
  uint32 null00;
  uint32 streams;
  uint32 cachedTextures;
  uint32 shaders;
  ArrayInv<uint16> unkIndices; // model indices??
  uint32 null01[7];
};

struct TGLDEntry {
  BBOX bounds;
  StreamEntry entry;
  uint32 unk01[6];
};

struct TerrainLODModel {
  Bounds bounds;
  float unk00;
  StreamEntry entry;
  uint32 unk01[2];
  float unk02[4];
};

struct Grass {
  Bounds bounds;
  float unk03[4];
  StreamEntry entry;
};

struct TerrainTexture {
  uint32 unk;
  uint32 size;
  uint32 offset;
  int32 uncachedID;
};

struct TerrainTextureHeader {
  Array<TerrainTexture> textures;
  uint32 numUsedUncachedTextures;
  uint32 null[4];
};

struct BVSCBlockEntry {
  DataSpan data;
  uint32 index;
  uint32 null[4];
};

struct BVSCBlock {
  uint32 hash;
  ArrayInv<BVSCBlockEntry> entries;
  DataSpan main;
};

struct Header : HeaderBase {
  Array<TerrainModel> terrainModels;
  Array<ObjectModel> objects;
  Array<EmbededHKX> hkColisions;
  Array<SkyboxModel> skyboxModels;
  uint32 null00[6];
  Array<StreamEntry> objectStreams;
  Array<ObjectTextureFile> objectTextures;
  Pointer<char> havokNames;
  Array<Grass> grass;
  Array<StreamEntry> unk00;
  Array<StreamEntry> unk01;
  Array<Pointer<char>> TGLDNames;
  Pointer<char> cachedTGLD;
  Array<TGLDEntry> TGLDs;
  Array<StreamEntry> terrainCachedTextures;
  Array<StreamEntry> terrainStreamingTextures;
  ArrayInv<BVSCBlock> BVSC;
  DataSpan LCMD;
  Array<StreamEntry> effects;
  Array<TerrainLODModel> terrainLODModels;
  uint32 null02;
  Array<StreamEntry> unk02;
  Array<StreamEntry> mapTerrainBuffers;
  Pointer<char> CEMS;
};
} // namespace V1

namespace V2 {
struct Header : HeaderBase {
  Array<TerrainModel> terrainModels;
  Array<ObjectModel> objects;
  uint32 null0;
  uint32 null1;
  Array<SkyboxModel> skyboxModels;
  uint32 unk01;
  uint32 null00[5];
  Array<StreamEntry> objectStreams;
  Array<ObjectTextureFile> objectTextures;
  uint32 null09;
  uint32 null10;
  uint32 null20;
  uint32 null30;
  uint32 null40;
  uint32 null50;
  uint32 null60;
  uint32 unk;
  uint32 null80;
  Pointer<char> cachedTGLD;
  uint32 null81;
  uint32 null91;
  Array<StreamEntry> terrainCachedTextures;
  uint32 null02;
  uint32 null12;
  uint32 null22;
  uint32 null32;
  uint32 null42;
  uint32 null52;
  uint32 null62;
  uint32 null72;
  uint32 null82;
  uint32 null92;
  uint32 null85;
  uint32 null95;
  uint32 null01;
  Array<StreamEntry> unk0;
  Pointer<char> nerd;
  uint32 null14;
  uint32 null24;
  uint32 null34;
  Pointer<char> lbigs;
  Pointer<char> dlcm;
  uint32 null64;
  uint32 null74;
  uint32 null84;
  uint32 null94;
  uint32 null87;
  uint32 null97;
  uint32 null04;
};
} // namespace V2

} // namespace MSMD
