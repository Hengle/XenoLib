/*  Xenoblade Engine Format Library
    Copyright(C) 2019-2022 Lukas Cone

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

#include "spike/except.hpp"
#include "xenolib/bc/anim.hpp"
#include "xenolib/bc/skel.hpp"
#include <cassert>
#include <cstring>

template <> void XN_EXTERN ProcessClass(BC::Header &item, ProcessFlags flags) {
  if (item.id != BC::ID) {
    throw es::InvalidHeaderError(item.id);
  }

  flags.NoProcessDataOut();
  flags.NoBigEndian();
  flags.NoAutoDetect();
  uint64 base = reinterpret_cast<uint64>(&item);
  reinterpret_cast<uint64 &>(item.data) += base;
  reinterpret_cast<uint64 &>(item.fixups) += base;

  for (size_t p = 0; p < item.numPointers; p++) {
    *reinterpret_cast<char **>(base + item.fixups[p]) += base;
  }
}

namespace BC::ANIM {
void CubicVector3Frame::Evaluate(Vector4A16 &out, float delta) {
  const Vector4A16 v1(delta, delta, delta, 1);
  const Vector4A16 v2(delta, delta, 1, 1);
  const Vector4A16 v3(delta, 1, 1, 1);
  const Vector4A16 vn = v1 * v2 * v3;
  Vector4A16 v4;

  for (size_t i = 0; i < 3; i++) {
    memcpy(&v4, elements[i].items, sizeof(Vector4A16));
    out[i] = v4.Dot(vn);
  }
}

void CubicQuatFrame::Evaluate(Vector4A16 &out, float delta) {
  const Vector4A16 v1(delta, delta, delta, 1);
  const Vector4A16 v2(delta, delta, 1, 1);
  const Vector4A16 v3(delta, 1, 1, 1);
  const Vector4A16 vn = v1 * v2 * v3;
  Vector4A16 v4;

  for (size_t i = 0; i < 4; i++) {
    memcpy(&v4, elements[i].items, sizeof(Vector4A16));
    out[i] = v4.Dot(vn);
  }
}

void AnimationTrack::Sample(uni::RTSValue &out, float frameTime) {
  auto Sample = [&](Vector4A16 &out, auto &frames) {
    decltype(frames.items) frame = nullptr;

    for (auto &f : frames) {
      if (f.frame > frameTime) {
        assert(&f > frames.begin());
        frame = (&f) - 1;
        break;
      }
    }

    if (!frame) {
      frame = std::prev(frames.end());
    }

    frame->Evaluate(out, frameTime - frame->frame);
  };

  Sample(out.rotation, rotation);
  Sample(out.translation, position);
  Sample(out.scale, scale);
}

} // namespace BC::ANIM
