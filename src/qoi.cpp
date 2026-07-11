#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <array>
#include <vector>

#include "image.cpp"

__inline__ int8_t cast_i2_to_i8(uint8_t x) {
  return static_cast<int8_t>(x << 6) >> 6;
}

__inline__ int8_t cast_i4_to_i8(uint8_t x) {
  return static_cast<int8_t>(x << 4) >> 4;
}

__inline__ int8_t cast_i6_to_i8(uint8_t x) {
  return static_cast<int8_t>(x << 2) >> 2;
}

#pragma pack(1)
struct QOIHeaders {
  std::array<char, 4> magic;
  uint32_t width;
  uint32_t height;
  uint8_t channels;
  uint8_t colorspace;
};

union Headers {
  QOIHeaders structure;
  std::array<uint8_t, 14> data;
};

struct Color {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
};

union ColorData {
  Color color;
  uint32_t data;
};

__inline__ uint8_t get_position_index(Color color) {
  return ((color.r * 3) + (color.g * 5) + (color.b * 7) + (color.a * 11)) % 64;
}

namespace qoi {
  static constexpr std::array<uint8_t, 8> endPattern = {0, 0, 0, 0, 0, 0, 0, 1};

  __inline__ int chech_header_validity(Headers headers) {
    std::array<char, 4> magic = {'q', 'o', 'i', 'f'};

    bool equal = std::equal(std::begin(magic), std::end(magic), std::begin(headers.structure.magic));
    return static_cast<int>(equal) - 1;
  }

  int decode(const std::string& source, Image &output) {
    // Currently requires there to be one commandline argument (the filename)
    std::ifstream file(source, std::ios::binary);
    if (!file || !file.is_open()) {
      std::cerr << "Error opening file!\n";

      return -1;
    }

    std::vector<uint8_t> data(
      (std::istreambuf_iterator<char>(file)),
      std::istreambuf_iterator<char>()
    );

    if (data.size() < 14) {
      std::cerr << "No header\n";
      return -1;
    }

    Headers headers = {};
    for (size_t i = 0; i < sizeof headers.data; ++i) {
      headers.data[i] = data[i];
    }

    if (chech_header_validity(headers) != 0) { return -1; }

    ColorData colors[64] = {};
    // Current color is defined to start with rgb of 0 and alpha of 1.
    ColorData current_color = { .color = { .r = 0, .g = 0, .b = 0, .a = 255 } };

    output.height = be32toh(headers.structure.height);
    output.width = be32toh(headers.structure.width);

    bool ended = false;
    uint32_t i = 14;
    while (i < data.size() - 7) {
      if (std::equal(endPattern.begin(), endPattern.end(), data.begin() + i)) {
        ended = true;
        break;
      }

      uint8_t b0 = data[i];

      switch (b0) {
        case 0xFF: i++;
          current_color.color.r = data[i++];
          current_color.color.g = data[i++];
          current_color.color.b = data[i++];
          current_color.color.a = data[i++];

          output.data.push_back(current_color.color.r);
          output.data.push_back(current_color.color.g);
          output.data.push_back(current_color.color.b);
          output.data.push_back(current_color.color.a);
          break;
        case 0xFE: i++;
          current_color.color.r = data[i++];
          current_color.color.g = data[i++];
          current_color.color.b = data[i++];

          output.data.push_back(current_color.color.r);
          output.data.push_back(current_color.color.g);
          output.data.push_back(current_color.color.b);
          output.data.push_back(current_color.color.a);
          break;
        case 0xC0 ... 0xFD:
          for (int r = 0; r < (b0 & 0x3F) + 1; ++r) {
            output.data.push_back(current_color.color.r);
            output.data.push_back(current_color.color.g);
            output.data.push_back(current_color.color.b);
            output.data.push_back(current_color.color.a);
          }

          i++;
          break;
        case 0x00 ... 0x3F:
          current_color = colors[data[i++] & 0b00111111];
          output.data.push_back(current_color.color.r);
          output.data.push_back(current_color.color.g);
          output.data.push_back(current_color.color.b);
          output.data.push_back(current_color.color.a);
          break;

        case 0x40 ... 0x7F:
          current_color.color.r =
              (uint8_t)(current_color.color.r + cast_i2_to_i8((data[i] & 0b00110000) >> 4));
          current_color.color.g =
              (uint8_t)(current_color.color.g + cast_i2_to_i8((data[i] & 0b00001100) >> 2));
          current_color.color.b =
              (uint8_t)(current_color.color.b + cast_i2_to_i8((data[i] & 0b00000011) >> 0));

          output.data.push_back(current_color.color.r);
          output.data.push_back(current_color.color.g);
          output.data.push_back(current_color.color.b);
          output.data.push_back(current_color.color.a);
          i++;
          break;
        case 0x80 ... 0xBF:
          current_color.color.r = (uint8_t)(current_color.color.r + cast_i6_to_i8(data[i] & 0b00111111));
          current_color.color.g = (uint8_t)(current_color.color.g + cast_i6_to_i8(data[i] & 0b00111111));
          current_color.color.b = (uint8_t)(current_color.color.b + cast_i6_to_i8(data[i] & 0b00111111));
          i++;

          // Failed to read second LUMA byte
          current_color.color.r =
              (uint8_t)(current_color.color.r - cast_i4_to_i8((data[i] & 0b11110000) >> 4));
          current_color.color.b =
              (uint8_t)(current_color.color.b - cast_i4_to_i8((data[i] & 0b00001111) >> 0));

          output.data.push_back(current_color.color.r);
          output.data.push_back(current_color.color.g);
          output.data.push_back(current_color.color.b);
          output.data.push_back(current_color.color.a);
          i++;
          break;
      }

      colors[get_position_index(current_color.color)] = current_color;
    }

    file.close();

    return -1 + (1 * static_cast<int>(ended));
  }

  // The encoder currently accepts only well formatted 8-bit color data with the
  // alpha channel intact.
  //
  // - uint8_t* output = reinterpret_cast<uint8_t*>((uint32_t*)malloc(size));
  // ret: IF positive THEN length of output ELSE error value
  int encode(const Image& img, std::vector<uint8_t> &output) {
    // Reserve worst case.
    // This step is mainly to avoid additional allocations.
    // The size comes from the maximum pixel encoding + header sizes.
    output.reserve((img.height * img.width * 5) + 14 + 8);

    std::vector<uint8_t> data = img.data;
    uint32_t size = img.height * img.width;

    // memcpy(output.data(), h.data.data(), h.data.size());
    output.push_back(static_cast<uint8_t>('q'));
    output.push_back(static_cast<uint8_t>('o'));
    output.push_back(static_cast<uint8_t>('i'));
    output.push_back(static_cast<uint8_t>('f'));
    // uint32_t width = htobe32(img.width)
    output.push_back(static_cast<uint8_t>((img.width >> 24) & 0xFF));
    output.push_back(static_cast<uint8_t>((img.width >> 16) & 0xFF));
    output.push_back(static_cast<uint8_t>((img.width >> 8) & 0xFF));
    output.push_back(static_cast<uint8_t>((img.width >> 0) & 0xFF));
    // uint32_t height = htobe32(img.height)
    output.push_back(static_cast<uint8_t>((img.height >> 24) & 0xFF));
    output.push_back(static_cast<uint8_t>((img.height >> 16) & 0xFF));
    output.push_back(static_cast<uint8_t>((img.height >> 8) & 0xFF));
    output.push_back(static_cast<uint8_t>((img.height >> 0) & 0xFF));
    output.push_back(4); // channels
    output.push_back(0); // colorspace

    std::array<ColorData, 64> colors = {};
    // Current color is defined to start with rgb of 0 and alpha of 1.
    ColorData previous_color = { .color = { .r = 0, .g = 0, .b = 0, .a = 255 } };

    int run = 0;
    const ColorData* pixels = reinterpret_cast<const ColorData *>(data.data());
    for (uint32_t i = 0; i < size; i++) {
      ColorData pixel = pixels[i];

      if (pixel.data == previous_color.data) {
        run++;
        // QOI run encode is setup as `11xxxxxx`. If run is equal to 63 or 64, the byte overlaps with RGB or RGBA commands. Therefore it is limited to 62.
        if (run == 62 || i == size - 1) {
            output.push_back(0xC0 | (run - 1));
            run = 0;
        }

        continue;
      }

      if (run > 0) {
        output.push_back(0xC0 | (run - 1));
        run = 0;
      }

      int8_t dr = (int8_t)pixel.color.r - (int8_t)previous_color.color.r;
      int8_t dg = (int8_t)pixel.color.g - (int8_t)previous_color.color.g;
      int8_t db = (int8_t)pixel.color.b - (int8_t)previous_color.color.b;
      int8_t da = (int8_t)pixel.color.a - (int8_t)previous_color.color.a;

      int32_t dr_dg = dr - dg;
      int32_t db_dg = db - dg;

      uint8_t pos = get_position_index(pixel.color);

      if (colors[pos].data == pixel.data) {
        output.push_back(pos);
      } else if (-2 <= dr && dr <= 1 && -2 <= dg && dg <= 1 && -2 <= db &&
                db <= 1 && da == 0) {
        output.push_back(0x40 | (dr + 2) << 4 | (dg + 2) << 2 | (db + 2));
      } else if (-32 <= dg && dg <= 31 && -8 <= dr_dg && dr_dg <= 7 &&
                -8 <= db_dg && db_dg <= 7 && da == 0) {
        output.push_back(0x80 | (dg + 32));
        output.push_back((dr_dg + 8) << 4 | (db_dg + 8));
      } else if (da == 0) {
        output.push_back(0xFE);
        output.push_back(pixel.color.r);
        output.push_back(pixel.color.g);
        output.push_back(pixel.color.b);
      } else {
        output.push_back(0xFF);
        output.push_back(pixel.color.r);
        output.push_back(pixel.color.g);
        output.push_back(pixel.color.b);
        output.push_back(pixel.color.a);
      }

      previous_color = pixel;
      colors[pos] = pixel;
    }

    for (auto b: endPattern) {
      output.push_back(b);
    }

    return 0;
  }
}
