/*  TEX2DDS
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
#include "datas/fileinfo.hpp"
#include "datas/master_printer.hpp"
#include "dds.hpp"
#include "project.h"
#include "xenolib/xbc1.hpp"

es::string_view filters[]{
    ".catex$", ".witex$", ".wismt$", ".calut$", ".witx$", {},
};

static AppInfo_s appInfo{
    AppInfo_s::CONTEXT_VERSION,
    AppMode_e::CONVERT,
    ArchiveLoadType::FILTERED,
    TEX2DDS_DESC " v" TEX2DDS_VERSION ", " TEX2DDS_COPYRIGHT "Lukas Cone",
    nullptr,
    filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

void AppProcessFile(std::istream &stream, AppContext *ctx) {
  BinReaderRef rd(stream);
  AFileInfo outPath(ctx->outFile);
  std::string texData;
  std::string hiData;

  if (outPath.GetExtension() == ".wismt") {
    auto folder = outPath.GetFolder();

    if (!folder.ends_with("tex/nx/m/")) {
      printwarning("Supplied wismt file folder is not tex/nx/m, skipping");
      return;
    }

    uint32 id;
    rd.Push();
    if (rd.Read(id); id != CompileFourCC("xbc1")) {
      printinfo("File is stream, not texture, skipping.");
      return;
    }

    rd.Pop();
    std::string buffer;
    rd.ReadContainer(buffer, rd.GetSize());
    texData = DecompressXBC1(buffer.data());

    auto hiStream =
        ctx->RequestFile(folder.to_string().replace(folder.size() - 2, 1, "h") +
                         outPath.GetFilenameExt().to_string());
    BinReaderRef hird(*hiStream.Get());
    hird.ReadContainer(buffer, hird.GetSize());
    hiData = DecompressXBC1(buffer.data());
  } else {
    rd.ReadContainer(texData, rd.GetSize());
  }

  if (auto hdr = MTXT::Mount(texData); hdr->id == MTXT::ID) {
    FByteswapper(*const_cast<MTXT::Header *>(hdr));
    BinWritter wr(outPath.GetFullPathNoExt().to_string() + ".dds");
    auto dds = ToDDS(hdr);
    wr.Write(dds);
    std::string outBuffer;
    DDS::Mips dummy;
    outBuffer.resize(dds.ComputeBufferSize(dummy));
    MTXT::DecodeMipmap(*hdr, texData.data(), outBuffer.data());
    wr.WriteContainer(outBuffer);
  } else if (auto hdr = LBIM::Mount(texData); hdr->id == LBIM::ID) {
    BinWritter wr(outPath.GetFullPathNoExt().to_string() + ".dds");

    if (!hiData.empty()) {
      auto mutHdr = const_cast<LBIM::Header *>(hdr);
      mutHdr->width *= 2;
      mutHdr->height *= 2;
    }

    auto dds = ToDDS(hdr);
    wr.Write(dds);
    std::string outBuffer;
    DDS::Mips dummy;
    outBuffer.resize(dds.ComputeBufferSize(dummy));
    LBIM::DecodeMipmap(*hdr, hiData.empty() ? texData.data() : hiData.data(),
                       outBuffer.data());
    wr.WriteContainer(outBuffer);
  } else {
    throw std::runtime_error("Not valid texture");
  }
}
