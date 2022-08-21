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
#include "datas/bincore_fwd.hpp"
#include "uni/model.hpp"
#include "uni/skeleton.hpp"
#include <memory>

namespace MXMD {
class Impl;

class Morph : public uni::VertexArray {
public:
  virtual es::string_view Name() const = 0;
  virtual size_t TargetVertexArrayIndex() const = 0;
};

using Morphs = uni::List<Morph>;

class Model : public uni::Model {
  public:
  virtual const Morphs *MorphTargets() const = 0;
};

class XN_EXTERN Wrap {
public:
  Wrap();
  Wrap(Wrap &&);
  ~Wrap();
  operator const uni::Skeleton *();
  operator const Model *();

  enum class ExcludeLoad {
    Model,
    Materials,
    LowTextures,
    TextureStreams,
    Shaders,
  };

  using ExcludeLoads = es::Flags<ExcludeLoad>;

  void Load(BinReaderRef main, BinReaderRef stream,
            ExcludeLoads excludeLoads = {});

private:
  friend class WrapFriend;
  std::unique_ptr<Impl> pi;
};

}; // namespace MXMD
