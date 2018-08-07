#include "texture.h"

#include <assert.h>
#include <iostream>
#include <algorithm>

using namespace std;

namespace CMU462 {

inline void uint8_to_float( float dst[4], unsigned char* src ) {
  uint8_t* src_uint8 = (uint8_t *)src;
  dst[0] = src_uint8[0] / 255.f;
  dst[1] = src_uint8[1] / 255.f;
  dst[2] = src_uint8[2] / 255.f;
  dst[3] = src_uint8[3] / 255.f;
}

inline void float_to_uint8( unsigned char* dst, float src[4] ) {
  uint8_t* dst_uint8 = (uint8_t *)dst;
  dst_uint8[0] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[0])));
  dst_uint8[1] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[1])));
  dst_uint8[2] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[2])));
  dst_uint8[3] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[3])));
}

void Sampler2DImp::generate_mips(Texture& tex, int startLevel) {

  // NOTE(sky):
  // The starter code allocates the mip levels and generates a level
  // map simply fills each level with a color that differs from its
  // neighbours'. The reference solution uses trilinear filtering
  // and it will only work when you have mipmaps.

  // Task 7: Implement this

  // check start level
  if ( startLevel >= tex.mipmap.size() ) {
    std::cerr << "Invalid start level";
  }

  // allocate sublevels
  int baseWidth  = tex.mipmap[startLevel].width;
  int baseHeight = tex.mipmap[startLevel].height;
  int numSubLevels = (int)(log2f( (float)max(baseWidth, baseHeight)));

  numSubLevels = min(numSubLevels, kMaxMipLevels - startLevel - 1);
  tex.mipmap.resize(startLevel + numSubLevels + 1);
  // MipLevel& mi = tex.mipmap[0];
  // for (int i = 0; i < tex.mipmap[0].height; i++) {
  //   for (int j = 0; j < tex.mipmap[0].width; j++) {
  //     for (int k = 0; k < 4; k++) {
  //       printf("%.2hhu ", mi.texels[4 * (mi.width * i + j) + k]);
  //     }
  //     printf("\n");
  //   }
  // }
  int width  = baseWidth;
  int height = baseHeight;
  for (int i = 1; i <= numSubLevels; i++) {

    MipLevel& level = tex.mipmap[startLevel + i];

    // handle odd size texture by rounding down
    width  = max( 1, width  / 2); assert(width  > 0);
    height = max( 1, height / 2); assert(height > 0);

    level.width = width;
    level.height = height;
    level.texels = vector<unsigned char>(4 * width * height);

  }

  // fill all 0 sub levels with interchanging colors
  Color colors[3] = { Color(1,0,0,1), Color(0,1,0,1), Color(0,0,1,1) };

  for(size_t i = 1; i < tex.mipmap.size(); ++i) {

    MipLevel& above_mip = tex.mipmap[i - 1];
    MipLevel& mip = tex.mipmap[i];
    // printf("above_mip: %d %d\n", above_mip.width, above_mip.height);
    // printf("mip: %d %d\n", mip.width, mip.height);

    for (int y = 0; y < mip.height; y++) {
      for (int x = 0; x < mip.width; x++) {
        for (int k = 0; k < 4; k++) {
          mip.texels[4 * (y * mip.width + x) + k] =
            (above_mip.texels[4 * (2 * y * above_mip.width + x * 2) + k] +
             above_mip.texels[4 * ((2 * y + 1) * above_mip.width + x * 2) + k] +
             above_mip.texels[4 * (2 * y * above_mip.width + x * 2 + 1) + k] +
             above_mip.texels[4 * ((2 * y + 1) * above_mip.width + x * 2 + 1) + k]) /4;
        }
      }
    }
  }
}

Color Sampler2DImp::sample_nearest(Texture& tex,
                                   float u, float v,
                                   int level) {

  // Task ?: Implement nearest neighbour interpolation

  // return magenta for invalid level
  if ( level >= tex.mipmap.size() ) {
    std::cerr << "Invalid start level";
  }
  // printf("%.2f %.2f\n", u, v);
  MipLevel& mip = tex.mipmap[level];
  int x = floor(mip.width * u);
  int y = floor(mip.height * v);
  Color color;
  uint8_to_float(&color.r, &mip.texels[4 * (mip.width * y + x)]);
  return color;
}

Color Sampler2DImp::sample_bilinear(Texture& tex,
                                    float u, float v,
                                    int level) {

  // Task ?: Implement bilinear filtering

  // return magenta for invalid level

  // map u,v to texel coords
  // do index math to get access to nearest texel and its neighbors
  // compute weight based on distance of u,v to texel center.
  // return weighted average of color components.
  if ( level >= tex.mipmap.size() ) {
    std::cerr << "Invalid start level";
  }

  MipLevel& mip = tex.mipmap[level];
  u = mip.width * u - 0.5;
  v = mip.height * v - 0.5;
  int x = floor(u);
  int y = floor(v);


  float u_ratio = u - x;
  float v_ratio = v - y;
  float u_opposite = 1 - u_ratio;
  float v_opposite = 1 - v_ratio;
  unsigned char result[4];
  for (int color_id = 0; color_id < 4; color_id++) {
    result[color_id] = (mip.texels[4 * (mip.width * y + x) + color_id] * u_opposite +
                        mip.texels[4 * (mip.width * y + x + 1) + color_id] * u_ratio) * v_opposite
                      +(mip.texels[4 * (mip.width * (y + 1) + x) + color_id] * u_opposite +
                        mip.texels[4 * (mip.width * (y + 1) + x + 1) + color_id] * u_ratio) * v_ratio;
  }

  Color color;
  uint8_to_float(&color.r, &result[0]);
  return color;

}

Color Sampler2DImp::sample_trilinear(Texture& tex,
                                     float u, float v,
                                     float u_scale, float v_scale) {

  // Task 8: Implement trilinear filtering

  // return magenta for invalid level

  // u_scale and v_scale help to determine the level as a float.
  // then bilinear sample the floor(level) and ceil(level).
  // then linear interpolate between those two colors based on where
  // the float level falls between floor and ceiling.

  float min_pixel_represtation = min(u_scale, v_scale);
  if (min_pixel_represtation >= tex.mipmap[0].height) {
    return sample_bilinear(tex, u, v, 0);
  }

  for (int level = 1; level < tex.mipmap.size(); level++) {
    if (min_pixel_represtation >= tex.mipmap[level].height) {
      // printf("level: %d\n", level);
      Color result_floor = sample_bilinear(tex, u, v, level);
      Color result_ceil = sample_bilinear(tex, u, v, level - 1);

      int size_floor = tex.mipmap[level].height;
      int size_ceil = tex.mipmap[level - 1].height;
      float ratio = (min_pixel_represtation - size_floor) / (size_ceil - size_floor);
      return result_floor * (1 - ratio) + result_ceil * ratio;
    }
  }
  return sample_bilinear(tex, u, v, tex.mipmap.size());
}

} // namespace CMU462
