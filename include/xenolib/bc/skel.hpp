/*  Xenoblade Engine Format Library
    Copyright(C) 2019-2023 Lukas Cone

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
#include "spike/uni/rts.hpp"
#include "xenolib/bc.hpp"

namespace BC::SKEL {
static constexpr uint32 ID = CompileFourCC("SKEL");

struct Bone {
  char *name;
  uint64 null;
};

struct Skeleton {
  Array<char> unk0;
  char *null;
  char *rootBoneName;
  Array<int16> boneLinks;
  Array<Bone> bones;
  Array<uni::RTSValue> boneTransforms;
  Array<char> unk1[6];
};

struct Header : Block {
  Skeleton *skel;
};
}; // namespace BC::SKEL
