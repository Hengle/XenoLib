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
#include "datas/binwritter.hpp"
#include "datas/except.hpp"
#include "datas/fileinfo.hpp"
#include "datas/master_printer.hpp"
#include "datas/reflector.hpp"
#include "nlohmann/json.hpp"
#include "project.h"
#include "xenolib/bc/asmb.hpp"

es::string_view filters[]{
    ".asm$",
    {},
};

static AppInfo_s appInfo{
    AppInfo_s::CONTEXT_VERSION,
    AppMode_e::CONVERT,
    ArchiveLoadType::FILTERED,
    ASM2JSON_DESC " v" ASM2JSON_VERSION ", " ASM2JSON_COPYRIGHT "Lukas Cone",
    nullptr,
    filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

REFLECT(ENUMERATION(BC::ASMB::StateType),
        EnumProxy("Animation", uint64(BC::ASMB::StateType::Animation)),
        EnumProxy("Blend", uint64(BC::ASMB::StateType::Blend)),
        EnumProxy("ExitAnim", uint64(BC::ASMB::StateType::ExitAnim)));

REFLECT(ENUMERATION(BC::ASMB::TransitionType),
        EnumProxy("Basic", uint64(BC::ASMB::TransitionType::Basic)),
        EnumProxy("Span", uint64(BC::ASMB::TransitionType::Span)),
        EnumProxy("Switch", uint64(BC::ASMB::TransitionType::Switch)));

REFLECT(ENUMERATION(BC::ASMB::BlendType),
        EnumProxy("Type1D", uint64(BC::ASMB::BlendType::Type1D)),
        EnumProxy("Type1DBasis", uint64(BC::ASMB::BlendType::Type1DBasis)));
REFLECT(ENUMERATION(BC::ASMB::VarParamType),
        EnumProxy("Int", uint64(BC::ASMB::VarParamType::Int)),
        EnumProxy("Float", uint64(BC::ASMB::VarParamType::Float)));

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

void ToJSON(BC::ASMB::StateFallback &f, nlohmann::json &jFallback) {
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

void ToJSON(BC::ASMB::StateTransition *e, nlohmann::json &jChild) {
  jChild["type"] = [&] {
    static auto refEnum = GetReflectedEnum<BC::ASMB::TransitionType>();
    auto found =
        std::find(refEnum->values, refEnum->values + refEnum->numMembers,
                  uint64(e->type));

    if (found >= refEnum->values + refEnum->numMembers) {
      assert(false);
      return "< ??? >";
    }
    return refEnum->names[std::distance(refEnum->values, found)];
  }();

  auto OutSValue = [](BC::ASMB::SValue &sval, nlohmann::json &node) {
    node["targetState"] = sval.targetName;
    node["unk0"] = sval.unk00;
    node["unk1"] = sval.unk01;
  };

  if (e->type == BC::ASMB::TransitionType::Basic) {
    auto sval = static_cast<BC::ASMB::StateTransitionBasic *>(e);
    OutSValue(*sval, jChild);
  } else if (e->type == BC::ASMB::TransitionType::Span) {
    auto sval = static_cast<BC::ASMB::StateTransitionSpans *>(e);
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

  } else if (e->type == BC::ASMB::TransitionType::Switch) {
    auto sval = static_cast<BC::ASMB::StateTransitionSwitch *>(e);
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

void ToJSON(BC::ASMB::StateAnim *animState, nlohmann::json &jState) {
  jState["animFilename"] = animState->animFilename;
  jState["groupFolderIndex"] = animState->groupFolderIndex;
  jState["isMirrored"] = animState->isMirrored;
  jState["unkFloat0"] = animState->unk02;
  jState["unkFloat1"] = animState->unk03;
  jState["unkFloat2"] = animState->unk04;
  assert(animState->null00 == 0);
}

void AppProcessFile(std::istream &stream, AppContext *ctx) {
  BinReaderRef rd(stream);

  {
    BC::Header hdr;
    rd.Push();
    rd.Read(hdr);
    rd.Pop();

    if (hdr.id != BC::ID) {
      throw es::InvalidHeaderError(hdr.id);
    }
  }

  std::string buffer;
  rd.ReadContainer(buffer, rd.GetSize());
  BC::Header *hdr = reinterpret_cast<BC::Header *>(buffer.data());
  ProcessClass(*hdr, {});

  if (hdr->data->id != BC::ASMB::ID) {
    throw es::InvalidHeaderError(hdr->data->id);
  }

  nlohmann::json main;
  auto asmb = static_cast<BC::ASMB::Header *>(hdr->data)->ass;

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

      auto OutStateKV = [](auto &node, BC::ASMB::StateKeyValue &kv) {
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

      if (f->type == BC::ASMB::StateType::Animation) {
        auto animState = static_cast<BC::ASMB::StateAnimation *>(f);
        ToJSON(animState, jState);
      } else if (f->type == BC::ASMB::StateType::Blend) {
        auto blend = static_cast<BC::ASMB::StateBlend *>(f)->blend;
        jState["blendType"] = FromEnum(blend->type);
        jState["varParamIndex"] = blend->varParamIndex;

        if (blend->type == BC::ASMB::BlendType::Type1D) {
          auto blend1D = static_cast<BC::ASMB::Blend1D *>(blend);
          auto &jAnims = jState["blends"];

          for (size_t index = 0; auto &b : blend1D->blends) {
            auto &jAnim = jAnims[index++];
            jAnim["triggerValue"] = b.triggerValue;
            ToJSON(&b.anim, jAnim);
          }
        } else if (blend->type == BC::ASMB::BlendType::Type1DBasis) {
          auto blend1D = static_cast<BC::ASMB::Blend1DBasis *>(blend);
          auto &jBlends = jState["blends"];
          assert(blend1D->anims.numItems == 1);

          ToJSON(blend1D->anims.begin(), jState["baseAnim"]);

          for (size_t index = 0; auto &b : blend1D->blends) {
            auto &jAnim = jBlends[index++];
            jAnim["triggerValue"] = b.triggerValue;
            ToJSON(&b.anim, jAnim);
          }
        }
      } else if (f->type == BC::ASMB::StateType::ExitAnim) {
        auto animState = static_cast<BC::ASMB::StateExitAnim *>(f);
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

  AFileInfo outPath(ctx->outFile);
  BinWritter_t<BinCoreOpenMode::Text> wr(
      outPath.GetFullPathNoExt().to_string() + ".json");
  wr.BaseStream() << std::setw(2) << main;
}
