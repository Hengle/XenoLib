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
#include "datas/flags.hpp"

#ifdef XN_EXPORT
#define XN_EXTERN ES_EXPORT
#elif defined(XN_IMPORT)
#define XN_EXTERN ES_IMPORT
#else
#define XN_EXTERN
#endif

enum class ProcessFlag {
  AutoDetectEndian,
  EnsureBigEndian,
  ProcessDataOut,
};

class ProcessFlags : public es::Flags<ProcessFlag> {
public:
  char *base = nullptr;
  uint32 userData = 0;
  using parent = es::Flags<ProcessFlag>;
  using parent::parent;

  void NoAutoDetect() const;
  void NoProcessDataOut() const;
  void NoBigEndian() const;
  void NoLittleEndian() const;
};

template <class C>
void XN_EXTERN ProcessClass(C &input, ProcessFlags flags = {
                                          ProcessFlag::AutoDetectEndian});
