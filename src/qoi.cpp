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
    if ((uint32_t)(headers.data[0]) != (uint32_t)(magic[0])) {
      return -1;
    }

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

  int decode(const char *source) {
    uint8_t raw_headers[14] = {};
    ColorData colors[64] = {};
    ColorData current_color = {
        .data = 0x000000FF}; // Current color is defined to start with rgb of 0
                            // and alpha of 1.

    // Currently requires there to be one commandline argument (the filename)
    std::ifstream file(source, std::ios::binary);
    if (!file || !file.is_open()) {
      std::cerr << "Error opening file!\n";

      return -1;
    }

    for (auto slot : raw_headers)
      file.read((char *)&slot, 1);

    Headers headers;
    memcpy(headers.data, raw_headers, sizeof headers.data);
    if (!chech_header_validity(headers)) {
      return -1;
    }

    uint8_t data;
    while (file.read((char *)&data, sizeof data)) {
      switch (data) {
      case 0b00000000 ... 0b00000000: // OP_INDEX
        current_color = colors[data & 0b00111111];
        break;
      case 0b01000000 ... 0b01000000: // OP_DIFF
        current_color.color.r =
            (uint8_t)(current_color.color.r +
                      cast_i2_to_i8((data & 0b00110000) >> 4));
        current_color.color.g =
            (uint8_t)(current_color.color.g +
                      cast_i2_to_i8((data & 0b00001100) >> 2));
        current_color.color.b =
            (uint8_t)(current_color.color.b +
                      cast_i2_to_i8((data & 0b00000011) >> 0));
        break;
      case 0b10000000 ... 0b10000000: // OP_LUMA
        data = cast_i6_to_i8(data & 0b00111111);
        current_color.color.r = (uint8_t)(current_color.color.r + data);
        current_color.color.g = (uint8_t)(current_color.color.g + data);
        current_color.color.b = (uint8_t)(current_color.color.b + data);

        // Failed to read second LUMA byte
        if (!file.read((char *)&data, sizeof data)) {
          return -1;
        }
        current_color.color.r =
            (uint8_t)(current_color.color.r -
                      cast_i4_to_i8((data & 0b11110000) >> 4));
        current_color.color.b =
            (uint8_t)(current_color.color.b -
                      cast_i4_to_i8((data & 0b00001111) >> 0));
        break;
      case 0b11000000 ... 0b11111100: // TODO: OP_RUN

        break;
      case 0b11111110: // OP_RGB
        // Failed to read first RGB byte
        if (!file.read((char *)&data, sizeof data)) {
          return -1;
        }
        current_color.color.r = data;

        // Failed to read second RGB byte
        if (!file.read((char *)&data, sizeof data)) {
          return -1;
        }
        current_color.color.g = data;

        // Failed to read third RGB byte
        if (!file.read((char *)&data, sizeof data)) {
          return -1;
        }
        current_color.color.b = data;

        break;
      case 0b11111111: // OP_RGBA
        // Failed to read first RGBA byte
        if (!file.read((char *)&data, sizeof data)) {
          return -1;
        }
        current_color.color.r = data;

        // Failed to read second RGBA byte
        if (!file.read((char *)&data, sizeof data)) {
          return -1;
        }
        current_color.color.g = data;

        // Failed to read third RGBA byte
        if (!file.read((char *)&data, sizeof data)) {
          return -1;
        }
        current_color.color.b = data;

        // Failed to read fourth RGBA byte
        if (!file.read((char *)&data, sizeof data)) {
          return -1;
        }
        current_color.color.a = data;

        break;
      }
    }

    file.close();

    return 0;
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

    std::cout << img.height << " - " << img.width << "\n";

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
