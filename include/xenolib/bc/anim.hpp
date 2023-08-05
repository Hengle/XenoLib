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

namespace BC::ANIM {
static constexpr uint32 ID = CompileFourCC("ANIM");

struct CubicCurve {
  float items[4];
};

struct CubicFrame {
  float frame;
};

struct CubicVector3Frame : CubicFrame {
  CubicCurve elements[3];

  void XN_EXTERN Evaluate(Vector4A16 &out, float delta);
};

struct CubicQuatFrame : CubicFrame {
  CubicCurve elements[4];

  void XN_EXTERN Evaluate(Vector4A16 &out, float delta);
};

struct AnimationTrack {
  Array<CubicVector3Frame> position;
  Array<CubicQuatFrame> rotation;
  Array<CubicVector3Frame> scale;

  // frameTime must be clamped [0, maxFrame]
  void XN_EXTERN Sample(uni::RTSValue &out, float frameTime);
};

struct MorphTracks {
  void *null00;
  char *name;
  uint32 null01;
  Array<float> track;
};

struct ControlTrack {
  MorphTracks *morphs;
  Array<int16> ids;
};

struct AnimationData {
  Array<char> null00;
  void *null01;
  void *null02;
  Array<int16> bones;
  Array<ControlTrack> controlTracks;
};

struct Event {
  float triggerFrame;
  char *groupName;
  char *eventName;
};

struct Header : Block {
  AnimationData *animData;
  Array<char> null00;
  void *null01;
  const char *animationName;
  int16 unk01[2];
  float frameRate;
  float frameTime;
  uint32 frameCount;
  Array<Event> events;
  void *null02;
  Array<AnimationTrack> tracks;
};
} // namespace BC::ANIM
