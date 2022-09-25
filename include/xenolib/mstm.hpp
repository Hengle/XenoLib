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

#pragma once
#include "core.hpp"
#include "datas/bincore_fwd.hpp"
#include "uni/model.hpp"

namespace MSTM {
class Impl;

class XN_EXTERN Wrap {
public:
  Wrap();
  Wrap(Wrap &&);
  ~Wrap();
  operator const uni::List<const uni::Model> *();

  enum class ExcludeLoad {
    Model,
    Materials,
    Shaders,
  };

  using ExcludeLoads = es::Flags<ExcludeLoad>;

  size_t ModelIndex(size_t id) const;

  void LoadV1(BinReaderRef main, ExcludeLoads excludeLoads = {});
  void LoadV2(BinReaderRef main, ExcludeLoads excludeLoads = {});

private:
  friend class WrapFriend;
  std::unique_ptr<Impl> pi;
};
} // namespace MSIM
