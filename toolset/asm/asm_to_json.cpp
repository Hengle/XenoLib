/*  ASM2JSON
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

#include "datas/app_context.hpp"
#include "datas/binreader_stream.hpp"
#include "datas/except.hpp"
#include "datas/master_printer.hpp"
#include "datas/reflector.hpp"
#include "nlohmann/json.hpp"
#include "project.h"
#include "xenolib/bc/asmb.hpp"

std::string_view filters[]{
    ".asm$",
};

static AppInfo_s appInfo{
    .filteredLoad = true,
    .header = ASM2JSON_DESC " v" ASM2JSON_VERSION ", " ASM2JSON_COPYRIGHT
                            "Lukas Cone",
    .filters = filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

namespace A1 = BC::ASMB::V1;
namespace A2 = BC::ASMB::V2;

REFLECT(ENUMERATION(A1::StateType),
        EnumProxy{"Animation", uint64(A1::StateType::Animation)},
        EnumProxy{"Blend", uint64(A1::StateType::Blend)},
        EnumProxy{"ExitAnim", uint64(A1::StateType::ExitAnim)});

REFLECT(ENUMERATION(A1::TransitionType),
        EnumProxy{"Basic", uint64(A1::TransitionType::Basic)},
        EnumProxy{"Span", uint64(A1::TransitionType::Span)},
        EnumProxy{"Switch", uint64(A1::TransitionType::Switch)});

REFLECT(ENUMERATION(A1::BlendType),
        EnumProxy{"Type1D", uint64(A1::BlendType::Type1D)},
        EnumProxy{"Type1DBasis", uint64(A1::BlendType::Type1DBasis)});

REFLECT(ENUMERATION(A1::VarParamType),
        EnumProxy{"Int", uint64(A1::VarParamType::Int)},
        EnumProxy{"Float", uint64(A1::VarParamType::Float)});

REFLECT(ENUMERATION(A2::StateType),
        EnumProxy{"Animation", uint64(A2::StateType::Animation)},
        EnumProxy{"Blend", uint64(A2::StateType::Blend)},
        EnumProxy{"EffectsOnly", uint64(A2::StateType::EffectsOnly)});

template <class E> const char *FromEnum(E en) {
  static auto refEnum = GetReflectedEnum<E>();
  auto found = std::find(refEnum->values, refEnum->values + refEnum->numMembers,
                         uint64(en));

  if (found >= refEnum->values + refEnum->numMembers) {
    assert(false);
    return "< ??? >";
  }
  return refEnum->names[std::distance(refEnum->values, found)];
}

void ToJSON(A1::StateFallback &f, nlohmann::json &jFallback) {
  jFallback["name"] = f.name;
  assert(f.unk00[0] < 2);
  assert(f.unk00[1] < 2);
  assert(f.unk00[0] == f.unk00[1]);
  jFallback["unkBool0"] = f.unk00[0] == 1;
  assert(f.unk01 < 2);
  jFallback["unkBool1"] = f.unk01 == 1;
  assert(f.unk03 < 2);
  jFallback["unkBool2"] = f.unk03 == 1;
  assert(f.unk04 < 2);
  jFallback["unkBool3"] = f.unk04 == 1;
  jFallback["unkFloat"] = f.unk02;

  assert(f.null00[0] == 0);
  assert(f.null00[1] == 0);
}

void ToJSON(A1::StateTransition *e, nlohmann::json &jChild) {
  jChild["type"] = FromEnum(e->type);
  auto OutSValue = [](A1::SValue &sval, nlohmann::json &node) {
    node["targetState"] = sval.targetName;
    node["unk0"] = sval.unk00;
    node["unk1"] = sval.unk01;
  };

  if (e->type == A1::TransitionType::Basic) {
    auto sval = static_cast<A1::StateTransitionBasic *>(e);
    OutSValue(*sval, jChild);
  } else if (e->type == A1::TransitionType::Span) {
    auto sval = static_cast<A1::StateTransitionSpans *>(e);
    auto &subs = jChild["spans"];

    for (size_t index = 0; auto &s : sval->spans) {
      auto &sub = subs[index++];
      sub["name"] = s.name;
      sub["targetState"] = s.targetName;
      sub["unkInt0"] = s.unk01;
      sub["unkFloat0"] = s.unk02;
      sub["begin"] = s.begin;
      sub["end"] = s.end;
      sub["unkBool0"] = s.unk03 == 1;
      sub["unkBool1"] = s.unk04 == 1;

      assert(s.unk03 < 2);
      assert(s.unk04 < 2);
      assert(s.null00 == 0);
      assert(s.null01[0] == 0);
      assert(s.null01[1] == 0);
    }

  } else if (e->type == A1::TransitionType::Switch) {
    auto sval = static_cast<A1::StateTransitionSwitch *>(e);
    jChild["varParamIndex"] = sval->varParamIndex;
    auto &subs = jChild["cases"];

    for (size_t index = 0; auto &s : sval->cases) {
      auto &sub = subs[index++];
      OutSValue(s.val, sub);
      sub["caseValue"] = s.caseId;
    }
  } else {
    printwarning("Undefined condition type: " << uint32(e->type)
                                              << " name: " << e->name);
  }
}

void ToJSON(A1::StateAnim *animState, nlohmann::json &jState) {
  jState["animFilename"] = animState->animFilename;
  jState["groupFolderIndex"] = animState->groupFolderIndex;
  jState["isMirrored"] = animState->isMirrored;
  jState["unkFloat0"] = animState->unk02;
  jState["unkFloat1"] = animState->unk03;
  jState["unkFloat2"] = animState->unk04;
  assert(animState->null00 == 0);
}

void ToJSON(A2::StateAnim *animState, nlohmann::json &jState) {
  jState["animFilename"] = animState->animFilename;
  if (animState->joint) {
    jState["joint"] = animState->joint;
  }

  assert(animState->null0 == 0);
  jState["unkInt0"] = animState->unk;
  jState["unkFloats"] = animState->unkf;
}

void ToJSON(A2::EVAAnim *animState, nlohmann::json &jState) {
  jState["animFilename"] = animState->animFilename;
  if (animState->joint) {
    jState["joint"] = animState->joint;
  }

  jState["unkInt0"] = animState->unk;
  jState["unkFloats"] = animState->unkf;
}

void ToJSON(A2::StateTransition *tran, nlohmann::json &jCond) {
  jCond["targetState"] = tran->targetName;
  jCond["unkFloats0"] = tran->unk;
  assert(tran->pad0 == -1U);
  assert(tran->pad1 == -1U);
  jCond["unkFloats1"] = tran->unk0;
  jCond["unkInt0"] = tran->unk1;
  jCond["unkInts0"] = tran->unk2;
  jCond["unkInt1"] = tran->unk4;
  jCond["unkInt2"] = tran->unk5;
}

template <class C>
void ParseChildren(C *f, nlohmann::json &jState, nlohmann::json &main) {
  auto &jChildren = jState["children"];
  auto &jConds = main["conditions"];

  for (size_t index = 0; auto e : f->children) {
    nlohmann::json node;
    ToJSON(e, node);

    if (!jConds.contains(e->name)) {
      jConds[e->name] = std::move(node);
      jChildren[index++] = e->name;
    } else {
      auto &mNode = jConds[e->name];

      if (mNode.is_array()) {
        int32 foundIndex = -1;

        for (auto &n : mNode) {
          if (node == n) {
            break;
          }
          foundIndex++;
        }

        if (foundIndex < 0) {
          foundIndex = mNode.size();
          mNode.emplace_back(std::move(node));
        }

        std::string nName = e->name;
        nName.push_back('[');
        nName.append(std::to_string(foundIndex));
        nName.push_back(']');
        jChildren[index++] = std::move(nName);

      } else {
        if (node != mNode) {
          nlohmann::json nNode;
          nNode.emplace_back(std::move(mNode));
          nNode.emplace_back(std::move(node));
          jConds[e->name] = std::move(nNode);

          std::string nName = e->name;
          nName.append("[1]");
          jChildren[index++] = std::move(nName);
        } else {
          jChildren[index++] = e->name;
        }
      }
    }
  }
}

void ToJSON(A1::Assembly *asmb, nlohmann::json &main) {
  main["schemaVersion"] = 1;
  main["formatVersion"] = 1;
  if (asmb->skeletonFilename) {
    main["skeletonFilename"] = asmb->skeletonFilename;
  }

  if (asmb->groupFolders.numItems) {
    auto &jFolders = main["groupFolders"];
    for (auto f : asmb->groupFolders) {
      jFolders.emplace_back(f);
    }
  }

  if (asmb->animationResources.numItems) {
    auto &jAnimResz = main["animationResources"];

    for (size_t index = 0; auto &f : asmb->animationResources) {
      auto &jRes = jAnimResz[index++];
      jRes["path"] = f.fileName;
      auto &jIds = jRes["usedInStates"];

      for (auto &i : f.usedInStates) {
        jIds.emplace_back(i.index);
        assert(i.unk1 == -1U);
      }
    }
  }

  if (asmb->keyValues.numItems) {
    auto &jKVs = main["keyValues"];
    for (size_t index = 0; auto f : asmb->keyValues) {
      auto &jKV = jKVs[index++];
      jKV["key"] = f.key;
      jKV["value"] = f.value;
    }
  }

  if (asmb->varParams.numItems) {
    auto &jKVs = main["varParams"];
    for (size_t index = 0; auto f : asmb->varParams) {
      auto &jKV = jKVs[index++];
      jKV["name"] = f.name;
      jKV["type"] = FromEnum(f.type);
    }
  }

  if (asmb->stateGraph.numItems) {
    auto &jStateNodes = main["stateNodes"];
    for (size_t index = 0; auto f : asmb->stateGraph) {
      auto &jState = jStateNodes[index];
      jState["id"] = index++;
      jState["name"] = f->name;
      jState["type"] = FromEnum(f->type);
      jState["unk"] = f->unk0;

      auto OutStateKV = [](auto &node, A1::StateKeyValue &kv) {
        node["name"] = kv.name;
        assert(kv.unk == 1);
        if (kv.value) {
          node["value"] = kv.value;
        }
      };

      if (f->enterEvents.numItems) {
        auto &jStartEvents = jState["enterEvents"];

        for (size_t index = 0; auto c : f->enterEvents) {
          auto &jStartEvent = jStartEvents[index++];
          OutStateKV(jStartEvent, c);
        }
      }

      if (f->exitEvents.numItems) {
        auto &jEndEvents = jState["exitEvents"];

        for (size_t index = 0; auto c : f->exitEvents) {
          auto &jEndEvent = jEndEvents[index++];
          OutStateKV(jEndEvent, c);
        }
      }

      if (f->events.numItems) {
        auto &jEvents = jState["events"];

        for (size_t index = 0; auto e : f->events) {
          auto &jEvent = jEvents[index++];
          jEvent["triggerFrame"] = e.triggerFrame;
          OutStateKV(jEvent, e.data);
        }
      }

      if (f->fallback) {
        ToJSON(*f->fallback, jState["fallback"]);
      }

      if (f->children.numItems) {
        ParseChildren(f, jState, main);
      }

      if (f->type == A1::StateType::Animation) {
        auto animState = static_cast<A1::StateAnimation *>(f);
        ToJSON(animState, jState);
      } else if (f->type == A1::StateType::Blend) {
        auto blend = static_cast<A1::StateBlend *>(f)->blend;
        jState["blendType"] = FromEnum(blend->type);
        jState["varParamIndex"] = blend->varParamIndex;

        if (blend->type == A1::BlendType::Type1D) {
          auto blend1D = static_cast<A1::Blend1D *>(blend);
          auto &jAnims = jState["blends"];

          for (size_t index = 0; auto &b : blend1D->blends) {
            auto &jAnim = jAnims[index++];
            jAnim["triggerValue"] = b.triggerValue;
            ToJSON(&b.anim, jAnim);
          }
        } else if (blend->type == A1::BlendType::Type1DBasis) {
          auto blend1D = static_cast<A1::Blend1DBasis *>(blend);
          auto &jBlends = jState["blends"];
          assert(blend1D->anims.numItems == 1);

          ToJSON(blend1D->anims.begin(), jState["baseAnim"]);

          for (size_t index = 0; auto &b : blend1D->blends) {
            auto &jAnim = jBlends[index++];
            jAnim["triggerValue"] = b.triggerValue;
            ToJSON(&b.anim, jAnim);
          }
        }
      } else if (f->type == A1::StateType::ExitAnim) {
        auto animState = static_cast<A1::StateExitAnim *>(f);
        jState["animFilename"] = animState->animFilename;
        jState["groupFolderIndex"] = animState->groupFolderIndex;
        jState["unkInt0"] = animState->unk01;
        jState["unkFloat0"] = animState->unk03;
        jState["unkFloat1"] = animState->unk04;
        jState["unkFloat2"] = animState->unk05;
        assert(animState->null00 == 0);
      }
    }
  }
}

void ToJSON(A2::StateBlendedAnimation *anim, nlohmann::json &node) {
  node["unk1"] = anim->unk1;
  node["triggerValue"] = anim->triggerValue;
  node["varParamIndex"] = anim->varParamIndex;
  node["type"] = anim->type;
  assert(anim->null00 == 0);

  if (anim->blendAnims.numItems) {
    auto &jExits = node["blendAnims"];

    for (size_t index = 0; auto &e : anim->blendAnims) {
      auto &jExit = jExits[index++];
      ToJSON(&e.anim, jExit);
      jExit["triggerValue"] = e.triggerValue;
      jExit["unkBool"] = e.unk;
    }
  }

  if (anim->subBlends.numItems) {
    auto &jExits = node["subBlends"];
    for (size_t index = 0; auto &e : anim->subBlends) {
      ToJSON(e, jExits[index++]);
    }
  }
}

void ToJSON(A2::EventData &evt, nlohmann::json &main) {
  assert(evt.null0 == 0);
  assert(evt.null1 == 0);
  assert(evt.null2 == 0);
  assert(evt.null3 == 0);
  assert(evt.null4 == 0);
  assert(evt.null5 == 0);
  assert(evt.null6 == 0);
  assert(evt.pad == -1U);
  main["unkInts0"] = evt.unk0;
  main["unkFloat0"] = evt.unk1;
  main["unkFloat1"] = evt.unk2;
  main["unkFloat2"] = evt.unk3;
  main["unkFloat3"] = evt.unk4;
  main["unkFloat4"] = evt.unk5;
  main["unkFloat5"] = evt.unk6;
  main["unkInt0"] = evt.unk7;
}

void ToJSON(A2::Assembly *asmb, nlohmann::json &main) {
  main["schemaVersion"] = 1;
  main["formatVersion"] = 2;

  assert(asmb->null00 == 0);
  assert(asmb->arr0.numItems == 0);

  if (asmb->groupFolders.numItems) {
    auto &jFolders = main["groupFolders"];
    for (auto f : asmb->groupFolders) {
      jFolders.emplace_back(f);
    }
  }

  if (asmb->fsmGroups.numItems) {
    auto &jStateGroups = main["stateGroups"];

    for (auto &s : asmb->fsmGroups) {
      auto &jStateGroup = jStateGroups[s.groupName];
      jStateGroup["entryState"] = s.stateName;
      if (s.sName) {
        jStateGroup["prefixName"] = s.sName;
      }

      if (s.stateGraph.numItems) {
        auto &jStateNodes = jStateGroup["stateNodes"];
        for (size_t index = 0; auto f : s.stateGraph) {
          auto &jState = jStateNodes[index];
          jState["id"] = index++;
          jState["name"] = f->name;
          jState["type"] = FromEnum(f->type);

          if (f->children.numItems) {
            ParseChildren(f, jState, main);
          }

          if (f->anims.numItems) {
            auto &jAnims = jState["additionalAnims"];

            for (size_t index = 0; auto &a : f->anims) {
              ToJSON(&a, jAnims[index++]);
            }
          }

          if (f->evaAnims.numItems) {
            auto &jAnims = jState["evaAnims"];

            for (size_t index = 0; auto &a : f->evaAnims) {
              ToJSON(&a, jAnims[index++]);
            }
          }

          if (f->enterEvents.numItems) {
            auto &jStartEvents = jState["enterEvents"];

            for (size_t index_ = 0; auto c : f->enterEvents) {
              auto &jStartEvent = jStartEvents[index_++];
              ToJSON(c, jStartEvent);
            }
          }

          if (f->exitEvents.numItems) {
            auto &jEndEvents = jState["exitEvents"];

            for (size_t index = 0; auto c : f->exitEvents) {
              auto &jEndEvent = jEndEvents[index++];
              ToJSON(c, jEndEvent);
            }
          }

          if (f->events.numItems) {
            auto &jEvents = jState["events"];

            for (size_t index = 0; auto e : f->events) {
              auto &jEvent = jEvents[index++];
              jEvent["triggerFrame"] = e.triggerFrame;
              jEvent["unkFrame"] = e.unkFrame;
              ToJSON(e.data, jEvent);
            }
          }

          if (f->type == A2::StateType::Animation) {
            auto animState = static_cast<A2::StateAnimation *>(f);
            ToJSON(animState, jState);
          } else if (f->type == A2::StateType::Blend) {
            auto animState = static_cast<A2::StateBlendedAnimations *>(f);
            auto &jAnims = jState["anims"];

            for (size_t index = 0; auto a : animState->anims) {
              ToJSON(a, jAnims[index++]);
            }
          }
        }
      }

      if (s.exportedConditions.numItems) {
        auto &jConds = jStateGroup["exportedConditions"];
        for (size_t index = 0; auto f : s.exportedConditions) {
          ToJSON(f, jConds[index++]);
        }
      }
    }
  }
}

void AppProcessFile(AppContext *ctx) {
  {
    BC::Header hdr;
    ctx->GetType(hdr);

    if (hdr.id != BC::ID) {
      throw es::InvalidHeaderError(hdr.id);
    }
  }

  std::string buffer = ctx->GetBuffer();
  BC::Header *hdr = reinterpret_cast<BC::Header *>(buffer.data());
  ProcessClass(*hdr, {});

  if (hdr->data->id != BC::ASMB::ID) {
    throw es::InvalidHeaderError(hdr->data->id);
  }

  nlohmann::json main;
  auto asmb = static_cast<BC::ASMB::Header *>(hdr->data);

  std::visit([&main](auto v) { ToJSON(v, main); }, asmb->Get());

  ctx->NewFile(ctx->workingFile.ChangeExtension(".json")).str
      << std::setw(2) << main;
}
