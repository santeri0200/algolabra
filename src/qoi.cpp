#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <array>
#include <vector>

#include "image.cpp"

namespace qoi {
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
    uint8_t data[14];
  };

  __inline__ int chech_header_validity(Headers headers) {
    char magic[4] = {'q', 'o', 'i', 'f'};

    // Magic does not match
    for (int i = 0; i < sizeof magic; ++i) if (headers.data[i] != magic[i]) return -1;
    return 0;
  }

  __inline__ uint8_t get_position_index(uint8_t r, uint8_t g, uint8_t b,
                                        uint8_t a) {
    return (r * 3 + g * 5 + b * 7 + a * 11) % 64;
  }

  __inline__ int8_t cast_i2_to_i8(uint8_t x) { return (int8_t)(x << 6) >> 6; }

  __inline__ int8_t cast_i8_to_i2(uint8_t x) {
    int8_t y = x & 0b10000000;
    int8_t z = x & 0b00000001;

    return (y >> 6) | z;
  }

  __inline__ int8_t cast_i4_to_i8(uint8_t x) { return (int8_t)(x << 4) >> 4; }

  __inline__ int8_t cast_i8_to_i4(uint8_t x) {
    int8_t y = x & 0b10000000;
    int8_t z = x & 0b00000111;

    return (y >> 4) | z;
  }

  __inline__ int8_t cast_i6_to_i8(uint8_t x) { return (int8_t)(x << 2) >> 2; }

  __inline__ int8_t cast_i8_to_i6(uint8_t x) {
    int8_t y = x & 0b10000000;
    int8_t z = x & 0b00011111;

    return (y >> 2) | z;
  }

  union ColorData {
    struct Color {
      uint8_t r;
      uint8_t g;
      uint8_t b;
      uint8_t a;
    } color;
    uint32_t data;
  };

  int decode(const char *source, Image &output) {
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
    for (int i = 0; i < sizeof headers.data; ++i) headers.data[i] = data[i];
    if (chech_header_validity(headers) != 0) { return -1; }

    ColorData colors[64] = {};
    ColorData current_color = { .color = { .a = 255 }}; // Current color is defined to start with rgb of 0
                                                         // and alpha of 1.

    output.height = be32toh(headers.structure.height);
    output.width = be32toh(headers.structure.width);

    bool ended = false;
    uint32_t i = 14;
    while (i < data.size() - 7) {
      uint8_t b0 = data[i + 0];
      uint8_t b1 = data[i + 1];
      uint8_t b2 = data[i + 2];
      uint8_t b3 = data[i + 3];
      uint8_t b4 = data[i + 4];
      uint8_t b5 = data[i + 5];
      uint8_t b6 = data[i + 6];
      uint8_t b7 = data[i + 7];

      if (
        b0 == 0x0 &&
        b1 == 0x0 &&
        b2 == 0x0 &&
        b3 == 0x0 &&
        b4 == 0x0 &&
        b5 == 0x0 &&
        b6 == 0x0 &&
        b7 == 0x1
      ) {
        ended = true;
        break;
      };

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

      colors[get_position_index(current_color.color.r, current_color.color.g, current_color.color.b, current_color.color.a)] = current_color;
    }

    file.close();

    return -1 + (1 * ended);
  }

  // The encoder currently accepts only well formatted 8-bit color data with the
  // alpha channel intact.
  //
  // - uint8_t* output = reinterpret_cast<uint8_t*>((uint32_t*)malloc(size));
  // ret: IF positive THEN length of output ELSE error value
  int encode(Image img, std::vector<uint8_t> &output) {
    output.resize(img.height * img.width * 5 + 14 + 8); // Reserve worst case

    std::vector<uint8_t> data = img.data;
    uint32_t size = img.height * img.width;

    QOIHeaders headers = {
      .magic = {'q', 'o', 'i', 'f'},
      .width = htobe32(img.width),
      .height = htobe32(img.height),
      .channels = 4,
      .colorspace = 0,
    };

    Headers h = {};
    h.structure = headers;

    memcpy(output.data(), h.data, sizeof(headers));

    uint32_t offset = 14;

    std::array<ColorData, 64> colors = {};
    ColorData previous_color = { .color = { .a = 255 }}; // Current color is defined to start with rgb of 0
                                                         // and alpha of 1.

    int run = 0;
    const ColorData* pixels = reinterpret_cast<const ColorData *>(data.data());
    for (int i = 0; i < size; i++) {
      ColorData pixel = pixels[i];

      if (pixel.data == previous_color.data) {
        run++;
        if (run == 62 || i == size - 1) {
            output[offset++] = 0xC0 | (run - 1);
            run = 0;
        }

        continue;
      }

      if (run > 0) {
        output[offset++] = 0xC0 | (run - 1);
        run = 0;
      }

      int32_t dr = pixel.color.r - previous_color.color.r;
      int32_t dg = pixel.color.g - previous_color.color.g;
      int32_t db = pixel.color.b - previous_color.color.b;
      int32_t da = pixel.color.a - previous_color.color.a;

      int32_t dr_dg = dr - dg;
      int32_t db_dg = db - dg;

      uint8_t pos = get_position_index(pixel.color.r, pixel.color.g,
                                      pixel.color.b, pixel.color.a);

      if (colors[pos].data == pixel.data) {
        output[offset++] = pos;
      } else if (-2 <= dr && dr <= 1 && -2 <= dg && dg <= 1 && -2 <= db &&
                db <= 1 && da == 0) {
        output[offset++] = 0x40 | dr << 4 | dg << 2 | db;
      } else if (-32 <= dg && dg <= 31 && -8 <= dr_dg && dr_dg <= 7 &&
                -8 <= db_dg && db_dg <= 7 && da == 0) {
        output[offset++] = 0x80 | dg;
        output[offset++] = dr_dg << 4 | db_dg;
      } else if (da == 0) {
        output[offset++] = 0xFE;
        output[offset++] = pixel.color.r;
        output[offset++] = pixel.color.g;
        output[offset++] = pixel.color.b;
      } else {
        output[offset++] = 0xFF;
        output[offset++] = pixel.color.r;
        output[offset++] = pixel.color.g;
        output[offset++] = pixel.color.b;
        output[offset++] = pixel.color.a;
      }

      previous_color = pixel;
      colors[pos] = pixel;
    }
    
    output[offset++] = 0;
    output[offset++] = 0;
    output[offset++] = 0;
    output[offset++] = 0;
    output[offset++] = 0;
    output[offset++] = 0;
    output[offset++] = 0;
    output[offset++] = 1;

    output.resize(offset);
    return offset;
  }
}
