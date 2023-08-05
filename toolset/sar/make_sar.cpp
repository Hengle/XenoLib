/*  SARCreate
    Copyright(C) 2022-2023 Lukas Cone

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

#define ES_COPYABLE_POINTER

#include "project.h"
#include "spike/app_context.hpp"
#include "spike/except.hpp"
#include "spike/io/binreader.hpp"
#include "spike/io/binwritter.hpp"
#include "spike/io/stat.hpp"
#include "spike/master_printer.hpp"
#include "spike/reflect/reflector.hpp"
#include "xenolib/sar.hpp"
#include <atomic>
#include <mutex>
#include <thread>

static struct SARCreate : ReflectorBase<SARCreate> {
  std::string extension = "sar";
  bool bigEndian = false;
} settings;

REFLECT(CLASS(SARCreate),
        MEMBER(extension, "e",
               ReflDesc{"Set output file extension. (common: sar, chr, mot)"}),
        MEMBERNAME(bigEndian, "big-endian", "e",
                   ReflDesc{"Output platform is big endian."}), );

static AppInfo_s appInfo{
    .header = SARCreate_DESC " v" SARCreate_VERSION ", " SARCreate_COPYRIGHT
                             "Lukas Cone",
    .settings = reinterpret_cast<ReflectorFriend *>(&settings),
};

AppInfo_s *AppInitModule() { return &appInfo; }

struct SarMakeContext : AppPackContext {
  std::string outSar;
  BinWritter_t<BinCoreOpenMode::NoBuffer> streamStore;
  std::vector<SAR::FileEntry> files;

  SarMakeContext() = default;
  SarMakeContext(const std::string &path, const AppPackStats &)
      : outSar(path), streamStore(path + ".data") {}
  SarMakeContext &operator=(SarMakeContext &&) = default;

  void SendFile(std::string_view path, std::istream &stream) override {
    if (path.size() >= sizeof(SAR::FileEntry::fileName)) {
      printwarning("Skipped (filename too long, >=52) " << path);
      return;
    }

    stream.seekg(0, std::ios::end);
    const size_t streamSize = stream.tellg();
    stream.seekg(0);

    SAR::FileEntry curFile;
    curFile.dataSize = streamSize;
    memcpy(curFile.fileName, path.data(), path.size());
    curFile.RecalcHash();

    {
      std::string buffer;
      buffer.resize(streamSize);
      stream.read(&buffer[0], streamSize);
      {
        static std::mutex mtx;
        std::lock_guard g(mtx);
        curFile.data.Reset(streamStore.Tell());
        files.emplace_back(curFile);
        streamStore.WriteContainer(buffer);
        streamStore.ApplyPadding();
      }
    }
  }

  void Finish() override {
    BinWritter wrm(outSar);
    BinWritterRef_e wr(wrm);
    wr.SwapEndian(settings.bigEndian);
    const size_t dataOffset =
        sizeof(SAR::Header) + files.size() * sizeof(SAR::FileEntry);
    SAR::Header hdr;
    hdr.entries.items.Reset(sizeof(SAR::Header));
    hdr.entries.numItems = files.size();
    hdr.version = 0x0101;
    hdr.data.Reset(dataOffset);
    wr.Write(hdr);

    for (auto &f : files) {
      f.data.Reset(f.data.RawValue() + dataOffset);
      wr.Write(f);
    }

    es::Dispose(streamStore);
    BinReader_t<BinCoreOpenMode::NoBuffer> rd(outSar + ".data");
    const size_t fSize = rd.GetSize();
    char buffer[0x40000];
    const size_t numBlocks = fSize / sizeof(buffer);
    const size_t restBytes = fSize % sizeof(buffer);

    for (size_t b = 0; b < numBlocks; b++) {
      rd.Read(buffer);
      wr.Write(buffer);
    }

    if (restBytes) {
      rd.ReadBuffer(buffer, restBytes);
      wr.WriteBuffer(buffer, restBytes);
    }

    es::RemoveFile(outSar + ".data");
  }
};

static thread_local SarMakeContext archive;

AppPackContext *AppNewArchive(const std::string &folder,
                              const AppPackStats &stats) {
  auto file = folder;
  while (file.back() == '/') {
    file.pop_back();
  }

  file += "." + settings.extension;
  return &(archive = SarMakeContext(file, stats));
}
