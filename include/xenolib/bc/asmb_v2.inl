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

namespace V2 {
struct StateTransition {
  char *name;
  char *targetName;
  uint32 nameHash;
  float unk[4];
  uint32 pad0;
  float unk0[2];
  uint32 unk1;
  int16 unk2[8];
  uint32 pad1;
  float unk3;
  uint32 unk4;
  uint32 unk5;
};

struct StateAnim {
  char *animFilename;
  char *joint;
  uint32 null0;
  uint32 unk;
  float unkf[4];
};

struct EVAAnim {
  char *animFilename;
  char *joint;
  uint32 unk;
  float unkf[4];
};

enum class StateType : uint32 {
  Animation,
  Blend,
  EffectsOnly,
};

struct EventData {
  uint32 null0;
  uint32 unk0[6];
  uint32 null1;
  float unk1;
  uint32 null2;
  float unk2;
  uint32 null3;
  float unk3;
  uint32 null4;
  float unk4;
  uint32 null5;
  float unk5;
  uint32 null6;
  float unk6;
  uint32 null7;
  uint32 unk7;
  uint32 pad;
};

struct Event {
  float triggerFrame;
  float unkFrame;
  EventData data;
};

struct StateBase {
  char *name;
  uint32 hash;
  StateType type;
  uint32 unk0[4];
  Array<StateTransition *> children;
  Array<EventData> enterEvents;
  Array<EventData> exitEvents;
  Array<Event> events;
  Array<EVAAnim> evaAnims;
  Array<StateAnim> anims;
};

struct StateAnimation : StateBase, StateAnim {};

struct AnimBlend {
  float triggerValue;
  bool unk;
  StateAnim anim;
};

struct StateBlendedAnimation {
  uint32 null00;
  float triggerValue;
  float unk1; // weight?
  uint16 varParamIndex;
  int8 type; // [0, 4]
  Array<StateBlendedAnimation *> subBlends;
  Array<AnimBlend> blendAnims;
};

struct StateBlendedAnimations : StateBase {
  Array<StateBlendedAnimation *> anims;
};

struct FSMGroup {
  Array<StateBase *> stateGraph;
  Array<StateTransition *> exportedConditions;
  char *groupName;
  char *stateName;
  char *sName;
};
struct Assembly {
  Array<char *> groupFolders;
  Array<FSMGroup> fsmGroups;
  uint64 null00;
  Array<char> arr0;
};
} // namespace V2
