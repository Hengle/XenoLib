#include "xenolib/msim.hpp"
#include "datas/binreader_stream.hpp"
#include "xenolib/internal/model.hpp"
#include "xenolib/internal/msim.hpp"

template <> void XN_EXTERN FByteswapper(MSIM::V1::InstancedModel &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(MSIM::V1::InstanceLOD &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(MSIM::V1::InstanceMatrix &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(MSIM::V1::InstancesHeader &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(MSIM::V1::Header &item, bool) {
  FArraySwapper(item);
  std::swap(item.unk00, item.unk01);
}

template <>
void XN_EXTERN ProcessClass(MSIM::V1::InstancesHeader &item,
                            ProcessFlags flags) {
  flags.NoAutoDetect();
  flags.NoLittleEndian();
  flags.NoProcessDataOut();
  flags.base = reinterpret_cast<char *>(&item);
  FByteswapper(item);
  es::FixupPointers(flags.base, item.clusters, item.modelLODs, item.matrices);

  for (auto &i : item.clusters) {
    FByteswapper(i);
  }
  for (auto &i : item.modelLODs) {
    FByteswapper(i);
  }
  for (auto &i : item.matrices) {
    FByteswapper(i);
  }
}

template <>
void XN_EXTERN ProcessClass(MSIM::V1::Header &item, ProcessFlags flags) {
  flags.NoAutoDetect();
  flags.NoLittleEndian();
  flags.NoProcessDataOut();
  flags.base = reinterpret_cast<char *>(&item);
  FByteswapper(item);
  es::FixupPointers(flags.base, item.bufferIndices, item.externalTextures,
                    item.instances, item.materials, item.models, item.shaders,
                    item.textureContainerIds);
  ProcessClass(*item.instances, flags);
  MSIM::Wrap::ExcludeLoads exLoads =
      static_cast<MSIM::Wrap::ExcludeLoads>(flags.userData);

  if (exLoads != MSIM::Wrap::ExcludeLoad::Model && item.models) {
    ProcessClass(*item.models, flags);
  } else {
    item.models.Reset();
  }

  if (exLoads != MSIM::Wrap::ExcludeLoad::Shaders && item.shaders) {
    ProcessClass(*item.shaders, flags);
  } else {
    item.shaders.Reset();
  }

  if (exLoads != MSIM::Wrap::ExcludeLoad::Materials && item.materials) {
    ProcessClass(*item.materials, flags);
  } else {
    item.materials.Reset();
  }

  for (auto &i : item.bufferIndices) {
    FByteswapper(i);
  }

  for (auto &i : item.externalTextures) {
    FByteswapper(i);
  }

  for (auto &i : item.textureContainerIds) {
    FByteswapper(i);
  }
}

namespace MSIM {
struct V1Skin : uni::Skin {
  std::vector<size_t> indices;
  V1::InstanceMatrix *matrices;

  size_t NumNodes() const override { return indices.size(); }
  uni::TransformType TMType() const override {
    return uni::TransformType::TMTYPE_MATRIX;
  }
  void GetTM(es::Matrix44 &out, size_t index) const override {
    memcpy(&out, &matrices[indices[index]].mtx, sizeof(MXMD::TransformMatrix));
  }
  size_t NodeIndex(size_t) const override {
    throw std::logic_error("Unused call");
  }

  operator uni::Element<const uni::Skin>() const {
    return uni::Element<const uni::Skin>{this, false};
  }
};

struct V1Model : MDO::V1Model {
  uni::VectorList<uni::Skin, V1Skin> skins;
  using MDO::V1Model::V1Model;

  uni::SkinsConst Skins() const override {
    return uni::Element<const uni::List<uni::Skin>>(&skins, false);
  }
};

struct VarModel {
  std::variant<V1Model, MDO::V3Model> model;
  size_t streamIndex;

  VarModel() = default;
  VarModel(VarModel &&) = default;
  VarModel(auto &&any, size_t si) : model{any}, streamIndex(si) {}

  operator uni::Element<const uni::Model>() const {
    return uni::Element<const uni::Model>(
        std::visit([](auto &item) -> const uni::Model * { return &item; },
                   model),
        false);
  }
};

class Impl {
public:
  std::string buffer;
  std::vector<std::string> streams;
  uni::VectorList<const uni::Model, VarModel> models;

  void LoadV1(BinReaderRef rd, Wrap::ExcludeLoads excludeLoads) {
    rd.ReadContainer(buffer, rd.GetSize());

    ProcessFlags flags{ProcessFlag::EnsureBigEndian};
    flags.userData = static_cast<uint32>(excludeLoads);

    auto main = reinterpret_cast<V1::Header *>(buffer.data());
    ProcessClass(*main, flags);

    if (MXMD::V1::Model *mod = main->models) {
      for (size_t index = 0; auto &m : mod->meshes) {
        models.storage.emplace_back(
            [&] {
              V1Model mdo;

              for (auto &p : m.primitives) {
                mdo.primitives.storage.emplace_back(p);
              }

              return mdo;
            }(),
            main->bufferIndices.begin()[index++]);
      }
    }

    if (V1::InstancesHeader *instances = main->instances) {
      for (size_t mIndex = 0; auto &m : instances->matrices) {
        auto &cluster = instances->clusters.begin()[m.lodIndex];

        for (size_t g = cluster.modelIndexStart;
             g < cluster.numModels + cluster.modelIndexStart; g++) {
          auto mdlIndex = instances->modelLODs.begin()[g].modelIndex;

          if (mdlIndex < 0) {
            mdlIndex ^= 0x80000000;
          }

          auto &model = std::get<V1Model>(models.storage.at(mdlIndex).model);

          if (model.skins.storage.empty()) {
            model.skins.storage.emplace_back().matrices =
                instances->matrices.items.Get();
          }

          auto &skin = model.skins.storage.front();
          skin.indices.emplace_back(mIndex);
        }

        mIndex++;
      }

      for (auto &m : instances->clusters) {
        for (size_t l = 0; l < m.numModels; l++) {
          auto mdlIndex =
              instances->modelLODs.begin()[m.modelIndexStart + l].modelIndex;
          if (mdlIndex < 0) {
            mdlIndex ^= 0x80000000;
          }
          auto &model = std::get<V1Model>(models.storage.at(mdlIndex).model);

          for (auto &p : model.primitives.storage) {
            p.lod = l;
          }
        }
      }
    }
  }

  void LoadV2(BinReaderRef rd, Wrap::ExcludeLoads excludeLoads) {
    throw std::logic_error("Mothod not implemented");
  }
};

Wrap::Wrap() : pi(std::make_unique<Impl>()) {}
Wrap::Wrap(Wrap &&) = default;
Wrap::~Wrap() = default;

void Wrap::LoadV1(BinReaderRef main, ExcludeLoads excludeLoads) {
  pi->LoadV1(main, excludeLoads);
}

void Wrap::LoadV2(BinReaderRef main, ExcludeLoads excludeLoads) {
  pi->LoadV2(main, excludeLoads);
}

Wrap::operator const uni::List<const uni::Model> *() { return &pi->models; }

size_t MSIM::Wrap::ModelIndex(size_t id) const {
  return pi->models.storage.at(id).streamIndex;
}

class WrapFriend : public Wrap {
public:
  using Wrap::pi;
};

} // namespace MSIM
