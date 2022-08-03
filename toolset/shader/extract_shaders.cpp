/*  SHDExtract
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
#include "datas/fileinfo.hpp"
#include "datas/master_printer.hpp"
#include "datas/reflector.hpp"
#include "latte/latte_disassembler.h"
#include "project.h"
#include "xenolib/internal/mxmd.hpp"
#include "xenolib/mths.hpp"
#include "xenolib/mxmd.hpp"
#include <sstream>

es::string_view filters[]{
    ".camdo$",
    ".cashd$",
    {},
};

static AppInfo_s appInfo{
    AppInfo_s::CONTEXT_VERSION,
    AppMode_e::EXTRACT,
    ArchiveLoadType::FILTERED,
    SHDExtract_DESC " v" SHDExtract_VERSION ", " SHDExtract_COPYRIGHT
                    "Lukas Cone",
    nullptr,
    filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

REFLECT(ENUMERATION(gx2::SamplerType), ENUM_MEMBER(gsampler1D),
        ENUM_MEMBER(gsampler2D), ENUM_MEMBER(gsampler3D),
        ENUM_MEMBER(gsamplerCube), );

#define ENUM_MEMBERNAME(type, name)                                            \
  EnumProxy { #name, static_cast < uint64>(enum_type::type) }

REFLECT(ENUMERATION(gx2::ShaderVarType), ENUM_MEMBERNAME(Void, void),
        ENUM_MEMBERNAME(Bool, bool), ENUM_MEMBERNAME(Int, int),
        ENUM_MEMBER(uint), ENUM_MEMBERNAME(Float, float),
        ENUM_MEMBERNAME(Double, double), ENUM_MEMBER(dvec2), ENUM_MEMBER(dvec3),
        ENUM_MEMBER(dvec4), ENUM_MEMBER(vec2), ENUM_MEMBER(vec3),
        ENUM_MEMBER(vec4), ENUM_MEMBER(bvec2), ENUM_MEMBER(bvec3),
        ENUM_MEMBER(bvec4), ENUM_MEMBER(ivec2), ENUM_MEMBER(ivec3),
        ENUM_MEMBER(ivec4), ENUM_MEMBER(uvec2), ENUM_MEMBER(uvec3),
        ENUM_MEMBER(uvec4), ENUM_MEMBER(mat2), ENUM_MEMBER(mat2x3),
        ENUM_MEMBER(mat2x4), ENUM_MEMBER(mat3x2), ENUM_MEMBER(mat3),
        ENUM_MEMBER(mat3x4), ENUM_MEMBER(mat4x2), ENUM_MEMBER(mat4x3),
        ENUM_MEMBER(mat4), ENUM_MEMBER(dmat2), ENUM_MEMBER(dmat2x3),
        ENUM_MEMBER(dmat2x4), ENUM_MEMBER(dmat3x2), ENUM_MEMBER(dmat3),
        ENUM_MEMBER(dmat3x4), ENUM_MEMBER(dmat4x2), ENUM_MEMBER(dmat4x3),
        ENUM_MEMBER(dmat4), )

void ExtractMTHS(char *buffer, AppExtractContext *ctx, es::string_view name) {
  auto hdr = reinterpret_cast<MTHS::Header *>(buffer);
  ProcessClass(*hdr);
  std::stringstream output;

  auto DumpSamplers = [&output](auto &node) {
    for (gx2::Sampler &s : node.samplers) {
      static const ReflectedEnum *en = GetReflectedEnum<gx2::SamplerType>();
      auto val = std::find(en->values, en->values + en->numMembers, s.type);
      output << "layout(binding = " << s.location << ") uniform "
             << en->names[std::distance(en->values, val)] << ' ' << s.name.Get()
             << ";\n";
    }
  };

  auto DumpAttributes = [&output](auto &node) {
    for (gx2::Attribute &s : node.attributes) {
      static const ReflectedEnum *en = GetReflectedEnum<gx2::ShaderVarType>();
      auto val =
          std::find(en->values, en->values + en->numMembers, uint64(s.type));
      output << "layout(location = " << s.location << ") in "
             << en->names[std::distance(en->values, val)] << ' '
             << s.name.Get();

      if (s.count > 1) {
        output << '[' << s.count << ']';
      }

      output << ";\n";
    }
  };

  auto DumpUniformBlocks = [&output](auto &node) {
    for (gx2::UniformBlock &s : node.uniformBlocks) {
      output << "layout(binding = " << s.offset << ") uniform " << s.name.Get()
             << "{\n"
             << "  char " << s.name.Get() << "_data[" << s.size << "];\n};\n";
    }
  };

  auto DumpUniformVars = [&output](auto &node) {
    for (gx2::UniformValue &s : node.uniformVars) {
      static const ReflectedEnum *en = GetReflectedEnum<gx2::ShaderVarType>();
      auto val =
          std::find(en->values, en->values + en->numMembers, uint64(s.type));
      if (s.blockIndex > -1) {
        output << "layout(binding = " << s.blockIndex
               << ", location = " << s.offset << ") uniform ";
      } else {
        output << "uniform ";
      }

      output << en->names[std::distance(en->values, val)] << ' '
             << s.name.Get();

      if (s.count > 1) {
        output << '[' << s.count << ']';
      }

      output << '\n';
    }
  };

  auto Disassemble = [&output](auto &node) {
    output << "\n// GLSL INCOMPATIBLE R600/700 ASSEMBLY\n";
    std::string disProg = latte::disassemble(
        std::span{node.program.items.Get(), node.program.numItems});
    output << disProg;
  };

  if (hdr->vertexShader) {
    MTHS::VertexShader *vsh = hdr->vertexShader;
    ctx->NewFile(name.to_string() + ".vert");
    DumpSamplers(*vsh);
    DumpAttributes(*vsh);
    DumpUniformVars(*vsh);
    DumpUniformBlocks(*vsh);
    Disassemble(*vsh);
    auto str = output.str();
    output = {};
    ctx->SendData(str);
  }

  if (hdr->fragmentShader) {
    ctx->NewFile(name.to_string() + ".frag");
    MTHS::FragmentShader *fsh = hdr->fragmentShader;
    DumpSamplers(*fsh);
    DumpUniformVars(*fsh);
    DumpUniformBlocks(*fsh);
    Disassemble(*fsh);
    auto str = output.str();
    output = {};
    ctx->SendData(str);
  }
}

template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

void ExtractMXMD(BinReaderRef rd, AppExtractContext *ctx) {
  MXMD::Wrap mxmd;
  using ex = MXMD::Wrap::ExcludeLoad;
  mxmd.Load(rd, {},
            {ex::Materials, ex::Model, ex::LowTextures, ex::TextureStreams});
  auto var = MXMD::GetVariantFromWrapper(mxmd);

  std::visit(overloaded{
                 [ctx, rd](MXMD::V1::Header &hdr) {
                   if (hdr.shaders) {
                     for (size_t index = 0; auto &s : hdr.shaders->shaders) {
                       // todo name from material + lod
                       auto idx = std::to_string(index++);
                       ExtractMTHS(s.data.items, ctx, idx);
                     }
                   }
                 },
                 [ctx, rd](MXMD::V2::Header &) {
                   throw std::runtime_error("Not implemented");
                 },
                 [ctx, rd](MXMD::V3::Header &) {
                   throw std::runtime_error("Not implemented");
                 },
             },
             var);
}

void AppExtractFile(std::istream &stream, AppExtractContext *ctx) {
  BinReaderRef rd(stream);
  uint32 id;
  rd.Push();
  rd.Read(id);
  rd.Pop();
  std::string buffer;

  if (id == MTHS::ID) {
    rd.ReadContainer(buffer, rd.GetSize());
    AFileInfo info(ctx->ctx->workingFile);
    ExtractMTHS(buffer.data(), ctx, info.GetFilename());
  } else if (id == MXMD::ID || id == MXMD::ID_BIG) {
    ExtractMXMD(rd, ctx);
  } else {
    throw es::InvalidHeaderError(id);
  }
}
