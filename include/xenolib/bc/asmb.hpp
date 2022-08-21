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
#include "xenolib/bc.hpp"

namespace BC::ASMB {
static constexpr uint32 ID = CompileFourCC("ASMB");

enum class VarParamType {
  Int,
  Float,
};

struct VarParam {
  VarParamType type;
  char *name;
};

struct UsedInState {
  uint32 index;
  uint32 unk1;
};

struct Animation {
  char *fileName;
  Array<UsedInState> usedInStates;
};

struct KeyValue {
  char *key;
  char *value;
};

enum class StateType : uint32 {
  Animation,
  ExitAnim,
  Blend,
};

enum class TransitionType : uint32 {
  Basic,
  Span,
  Switch,
};

struct StateTransition {
  char *name;
  TransitionType type;
};

struct SValue {
  char *targetName;
  float unk00[6]; // delays, offsets, etc?
  uint32 unk01[2];
};

struct StateTransitionBasic : StateTransition, SValue {};

struct StateTransitionSpan {
  char *name;
  uint32 null00;
  char *targetName;
  float begin;
  float end;
  uint32 unk01; // 0 or 3
  uint32 null01[2];
  float unk02;
  uint32 unk03;
  uint32 unk04;
};

struct StateTransitionSpans : StateTransition {
  Array<StateTransitionSpan> spans;
};

struct StateTransitionCase {
  uint32 caseId;
  SValue val;
};

struct StateTransitionSwitch : StateTransition {
  uint32 varParamIndex;
  Array<StateTransitionCase> cases;
};

struct StateFallback {
  char *name;
  uint32 unk00[2];
  uint32 unk01;
  uint32 null00[2];
  float unk02;
  uint32 unk03;
  uint32 unk04;
};

struct StateKeyValue {
  char *name;
  uint32 unk;
  char *value;
};

struct Event {
  float triggerFrame;
  StateKeyValue data;
};

struct StateBase {
  StateType type;
  char *name;
  uint32 unk0[3];
  Array<StateTransition *> children;
  StateFallback *fallback;
  Array<StateKeyValue> enterEvents;
  Array<StateKeyValue> exitEvents;
  Array<Event> events;
};

struct StateAnim {
  char *animFilename;
  uint32 groupFolderIndex;
  uint32 isMirrored;
  float unk02;
  float unk03; // speed?
  float null00;
  float unk04;
};

struct StateAnimation : StateBase, StateAnim {};

enum class BlendType {
  Type1D = 2,
  Type1DBasis = 4,
};

struct Blend {
  BlendType type;
  uint32 varParamIndex;
};

struct BlendAnim {
  float triggerValue;
  StateAnim anim;
};

struct Blend1D : Blend {
  Array<BlendAnim> blends;
};

struct Blend1DBasis : Blend {
  Array<StateAnim> anims;
  Array<BlendAnim> blends;
};

struct StateBlend : StateBase {
  Blend *blend;
};

struct StateExitAnim : StateBase {
  char *animFilename;
  uint32 groupFolderIndex;
  uint32 unk01;
  uint32 null00;
  float unk03;
  float unk04;
  float unk05;
};

struct Assembly {
  uint32 unk;
  Array<char *> groupFolders;
  Array<StateBase *> stateGraph;
  Array<VarParam> varParams;
  Array<Animation> animationResources;
  uint64 null00;
  Array<KeyValue> keyValues;
};

struct Header : Block {
  Assembly *ass;
};

} // namespace BC::ASMB
