#include "xenolib/mstm.hpp"
#include "datas/binreader_stream.hpp"
#include "xenolib/internal/model.hpp"
#include "xenolib/internal/mstm.hpp"

template <>
void XN_EXTERN FByteswapper(MSTM::V1::TerrainBufferLookup &item, bool) {
  FArraySwapper(item);
}

template <>
void XN_EXTERN FByteswapper(MSTM::V1::TerrainBufferLookupHeader &item, bool) {
  FArraySwapper(item);
}

template <> void XN_EXTERN FByteswapper(MSTM::V1::Header &item, bool) {
  FArraySwapper(item);
  std::swap(item.unk00, item.unk01);
}

template <>
void XN_EXTERN ProcessClass(MSTM::V1::TerrainBufferLookupHeader &item,
                            ProcessFlags flags) {
  flags.NoAutoDetect();
  flags.NoLittleEndian();
  flags.NoProcessDataOut();
  flags.base = reinterpret_cast<char *>(&item);
  FByteswapper(item);
  es::FixupPointers(flags.base, item.bufferLookups, item.meshIndices);

  for (auto &i : item.bufferLookups) {
    FByteswapper(i);
  }
  for (auto &i : item.meshIndices) {
    FByteswapper(i);
  }
}

template <>
void XN_EXTERN ProcessClass(MSTM::V1::Header &item, ProcessFlags flags) {
  flags.NoAutoDetect();
  flags.NoLittleEndian();
  flags.NoProcessDataOut();
  flags.base = reinterpret_cast<char *>(&item);
  FByteswapper(item);
  es::FixupPointers(flags.base, item.buffers, item.externalTextures,
                    item.materials, item.models, item.shaders,
                    item.textureContainerIds);
  ProcessClass(*item.buffers, flags);
  MSTM::Wrap::ExcludeLoads exLoads =
      static_cast<MSTM::Wrap::ExcludeLoads>(flags.userData);

  if (exLoads != MSTM::Wrap::ExcludeLoad::Model && item.models) {
    ProcessClass(*item.models, flags);
  } else {
    item.models.Reset();
  }

  if (exLoads != MSTM::Wrap::ExcludeLoad::Shaders && item.shaders) {
    ProcessClass(*item.shaders, flags);
  } else {
    item.shaders.Reset();
  }

  if (exLoads != MSTM::Wrap::ExcludeLoad::Materials && item.materials) {
    ProcessClass(*item.materials, flags);
  } else {
    item.materials.Reset();
  }

  for (auto &i : item.externalTextures) {
    FByteswapper(i);
  }

  for (auto &i : item.textureContainerIds) {
    FByteswapper(i);
  }
}

namespace MSTM {
struct VarModel {
  std::variant<MDO::V1Model, MDO::V3Model> model;
  size_t streamIndex;

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

  void Load(BinReaderRef rd, Wrap::ExcludeLoads excludeLoads) {
    rd.ReadContainer(buffer, rd.GetSize());

    ProcessFlags flags{ProcessFlag::EnsureBigEndian};
    flags.userData = static_cast<uint32>(excludeLoads);

    auto main = reinterpret_cast<V1::Header *>(buffer.data());
    ProcessClass(*main, flags);

    if (MXMD::V1::Model *mod = main->models) {
      V1::TerrainBufferLookupHeader *buffers = main->buffers;

      for (size_t index = 0; auto &m : mod->meshes) {
        const size_t lookupIndexBig = buffers->meshIndices.begin()[index++];
        const size_t numBuffers = buffers->bufferLookups.numItems;
        const size_t lookupIndex = lookupIndexBig % numBuffers;
        const size_t lod = lookupIndexBig / numBuffers;
        const size_t bufferIndex =
            buffers->bufferLookups.begin()[lookupIndex].bufferIndex[lod];

        models.storage.emplace_back(
            [&] {
              MDO::V1Model mdo;

              for (auto &p : m.primitives) {
                mdo.primitives.storage.emplace_back(p).lod = lod;
              }

              return mdo;
            }(),
            bufferIndex);
      }
    }
  }
};

Wrap::Wrap() : pi(std::make_unique<Impl>()) {}
Wrap::Wrap(Wrap &&) = default;
Wrap::~Wrap() = default;

void Wrap::Load(BinReaderRef main, ExcludeLoads excludeLoads) {
  pi->Load(main, excludeLoads);
}

Wrap::operator const uni::List<const uni::Model> *() { return &pi->models; }

size_t MSTM::Wrap::ModelIndex(size_t id) const {
  return pi->models.storage.at(id).streamIndex;
}

class WrapFriend : public Wrap {
public:
  using Wrap::pi;
};

} // namespace MSTM
