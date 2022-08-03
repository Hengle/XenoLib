// From Decaf project
// license in 3rd_party/decaf

#pragma once
#include "datas/pointer.hpp"

namespace gx2 {
template <class C> using Pointer = es::PointerX86<C>;

enum TileMode {
  Default = 0,
  LinearAligned,
  Tiled1DThin1,
  Tiled1DThick,
  Tiled2DThin1,
  Tiled2DThin2,
  Tiled2DThin4,
  Tiled2DThick,
  Tiled2BThin1,
  Tiled2BThin2,
  Tiled2BThin4,
  Tiled2BThick,
  Tiled3DThin1,
  Tiled3DThick,
  Tiled3BThin1,
  Tiled3BThick,
  LinearSpecial,
  DefaultBadAlign = 0x20,
};

enum SurfaceDim {
  Texture1D,
  Texture2D,
  Texture3D,
  TextureCube,
  Texture1DArray,
  Texture2DArray,
  Texture2DMSAA,
  Texture2DMSAAArray,
};

enum AAMode {
  Mode1X,
  Mode2X,
  Mode4X,
  Mode8X,
};

enum SurfaceFormat {
  INVALID = 0x00,
  UNORM_R4_G4 = 0x02,
  UNORM_R4_G4_B4_A4 = 0x0b,
  UNORM_R8 = 0x01,
  UNORM_R8_G8 = 0x07,
  UNORM_R8_G8_B8_A8 = 0x01a,
  UNORM_R16 = 0x05,
  UNORM_R16_G16 = 0x0f,
  UNORM_R16_G16_B16_A16 = 0x01f,
  UNORM_R5_G6_B5 = 0x08,
  UNORM_R5_G5_B5_A1 = 0x0a,
  UNORM_A1_B5_G5_R5 = 0x0c,
  UNORM_R24_X8 = 0x011,
  UNORM_A2_B10_G10_R10 = 0x01b,
  UNORM_R10_G10_B10_A2 = 0x019,
  UNORM_BC1 = 0x031,
  UNORM_BC2 = 0x032,
  UNORM_BC3 = 0x033,
  UNORM_BC4 = 0x034,
  UNORM_BC5 = 0x035,
  UNORM_NV12 = 0x081,
  UINT_R8 = 0x101,
  UINT_R8_G8 = 0x107,
  UINT_R8_G8_B8_A8 = 0x11a,
  UINT_R16 = 0x105,
  UINT_R16_G16 = 0x10f,
  UINT_R16_G16_B16_A16 = 0x11f,
  UINT_R32 = 0x10d,
  UINT_R32_G32 = 0x11d,
  UINT_R32_G32_B32_A32 = 0x122,
  UINT_A2_B10_G10_R10 = 0x11b,
  UINT_R10_G10_B10_A2 = 0x119,
  UINT_X24_G8 = 0x111,
  UINT_G8_X24 = 0x11c,
  SNORM_R8 = 0x201,
  SNORM_R8_G8 = 0x207,
  SNORM_R8_G8_B8_A8 = 0x21a,
  SNORM_R16 = 0x205,
  SNORM_R16_G16 = 0x20f,
  SNORM_R16_G16_B16_A16 = 0x21f,
  SNORM_R10_G10_B10_A2 = 0x219,
  SNORM_BC4 = 0x234,
  SNORM_BC5 = 0x235,
  SINT_R8 = 0x301,
  SINT_R8_G8 = 0x307,
  SINT_R8_G8_B8_A8 = 0x31a,
  SINT_R16 = 0x305,
  SINT_R16_G16 = 0x30f,
  SINT_R16_G16_B16_A16 = 0x31f,
  SINT_R32 = 0x30d,
  SINT_R32_G32 = 0x31d,
  SINT_R32_G32_B32_A32 = 0x322,
  SINT_R10_G10_B10_A2 = 0x319,
  SRGB_R8_G8_B8_A8 = 0x41a,
  SRGB_BC1 = 0x431,
  SRGB_BC2 = 0x432,
  SRGB_BC3 = 0x433,
  FLOAT_R32 = 0x80e,
  FLOAT_R32_G32 = 0x81e,
  FLOAT_R32_G32_B32_A32 = 0x823,
  FLOAT_R16 = 0x806,
  FLOAT_R16_G16 = 0x810,
  FLOAT_R16_G16_B16_A16 = 0x820,
  FLOAT_R11_G11_B10 = 0x816,
  FLOAT_D24_S8 = 0x811,
  FLOAT_X8_X24 = 0x81c,
};

enum SamplerType {
  gsampler1D,
  gsampler2D,
  gsampler3D,
  gsamplerCube,
};

struct Sampler {
  Pointer<char> name;
  SamplerType type;
  uint32 location;
};

enum class ShaderVarType {
  Void,
  Bool,
  Int,
  uint,
  Float,
  Double,
  dvec2,
  dvec3,
  dvec4,
  vec2,
  vec3,
  vec4,
  bvec2,
  bvec3,
  bvec4,
  ivec2,
  ivec3,
  ivec4,
  uvec2,
  uvec3,
  uvec4,
  mat2,
  mat2x3,
  mat2x4,
  mat3x2,
  mat3,
  mat3x4,
  mat4x2,
  mat4x3,
  mat4,
  dmat2,
  dmat2x3,
  dmat2x4,
  dmat3x2,
  dmat3,
  dmat3x4,
  dmat4x2,
  dmat4x3,
  dmat4,
};

struct UniformValue {
  Pointer<char> name;
  ShaderVarType type;
  uint32 count;
  uint32 offset;
  int32 blockIndex;
};

struct Attribute {
  Pointer<char> name;
  ShaderVarType type;
  uint32 count;
  uint32 location;
};

struct UniformBlock {
  Pointer<char> name;
  uint32 offset;
  uint32 size;
};

enum class ShaderMode {
  UniformRegister,
  UniformBlock,
  GeometryShader,
  ComputeShader,
};

} // namespace gx2
