#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#include "image.cpp"

uint32_t bgra_to_rgba(uint32_t x) {
  uint32_t a = (x >> 24) & 0xFF;
  uint32_t b = (x >> 16) & 0xFF;
  uint32_t g = (x >>  8) & 0xFF;
  uint32_t r = (x >>  0) & 0xFF;

  return (r << 24) | (g << 16) | (b << 8) | a;
}

uint32_t rgb_to_rgba(uint32_t x) {
  return x | 0xFF;
}

uint32_t bgr_to_rgba(uint32_t x) {
  uint32_t b = (x >> 16) & 0xFF;
  uint32_t g = (x >>  8) & 0xFF;
  uint32_t r = (x >>  0) & 0xFF;

  return (r << 24) | (g << 16) | (b << 8) | 0xFF;
}

uint32_t abgr_to_rgba(uint32_t x) {
  uint32_t a = (x >> 24) & 0xFF;
  uint32_t b = (x >> 16) & 0xFF;
  uint32_t g = (x >>  8) & 0xFF;
  uint32_t r = (x >>  0) & 0xFF;

  return (r << 24) | (g << 16) | (b << 8) | a;
}

namespace bmp {
  int decode(const std::string& source, Image &output) {
    // Currently requires there to be one commandline argument (the filename)
    std::ifstream file(source, std::ios::binary);
    if (!file || !file.is_open()) {
      std::cerr << "Error opening file! \"" << source << "\"\n";

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

    uint16_t identity = 0;
    memcpy(&identity, &data[0], sizeof(identity));
    if (identity != 0x4D42) {
      std::cerr << "Wrong identity\n";
      return -1;
    }

    uint32_t size = 0;
    uint32_t offset = 0;

    memcpy(&size, &data[2], sizeof(size));
    memcpy(&offset, &data[10], sizeof(offset));

    uint32_t dib_size = 0;
    memcpy(&dib_size, &data[14], sizeof(dib_size));

    uint32_t colortable_size = 0;
    uint32_t nbits = 0;

    int32_t width = 0;
    int32_t height = 0;

    switch (dib_size) {
      case 40:
        memcpy(&width, &data[18], sizeof(width));
        memcpy(&height, &data[22], sizeof(height));
        memcpy(&colortable_size, &data[46], sizeof(colortable_size));
        memcpy(&nbits, &data[28], sizeof(nbits));
        if (colortable_size == 0) {
          colortable_size = (uint32_t)1 << nbits;
        }
        break;
      default:
        std::cerr << "Not supported dib headers\n";
        return -3;
    }

    output.width = width;
    output.height = height;

    uint32_t row_size = ((width * nbits + 31) & ~31) / 8; // Row size in bits, divided by input datatype width

    auto palette = reinterpret_cast<const uint32_t *>(&data[14 + dib_size]);
    auto ptr = &data[offset];

    uint32_t abs_heigh = std::abs(height);
    for (uint32_t y = 0; y < abs_heigh; ++y) {
      int yy = (0 < height)
        ? (abs_heigh - y - 1)
        : y;

      auto row = &ptr[yy * row_size];
      for (int x = 0; x < width; ++x) {
        if (nbits == 1) {
          uint32_t slot = x / 8;
          uint32_t shift = x % 8;

          uint32_t index = (row[slot] >> shift) & 0x1;
          uint32_t value = bgr_to_rgba(palette[index]);
          output.data.push_back((value >> 24) & 0xFF);
          output.data.push_back((value >> 16) & 0xFF);
          output.data.push_back((value >>  8) & 0xFF);
          output.data.push_back((value >>  0) & 0xFF);
        } else if (nbits == 2) {
          std::cerr << "Unsupported bit depth: 2\n";
          return -2;
        } else if (nbits == 4) {
          std::cerr << "Unsupported bit depth: 4\n";
          return -2;
        } else if (nbits == 8) {
          std::cerr << "Unsupported bit depth: 8\n";
          return -2;
        } else if (nbits == 16) {
          std::cerr << "Unsupported bit depth: 16\n";
          return -2;
        } else if (nbits == 24) {
          uint32_t a = row[x * 3 + 0];
          uint32_t b = row[x * 3 + 1];
          uint32_t c = row[x * 3 + 2];

          uint32_t value = bgr_to_rgba((a << 16) | (b << 8) | c);
          output.data.push_back((value >> 24) & 0xFF);
          output.data.push_back((value >> 16) & 0xFF);
          output.data.push_back((value >>  8) & 0xFF);
          output.data.push_back((value >>  0) & 0xFF);
        } else if (nbits == 32) {
          uint32_t a = row[x * 4 + 0];
          uint32_t b = row[x * 4 + 1];
          uint32_t c = row[x * 4 + 2];
          uint32_t d = row[x * 4 + 3];

          uint32_t value = abgr_to_rgba((a << 24) | (b << 16) | (c << 8) | d);

          output.data.push_back((value >> 24) & 0xFF);
          output.data.push_back((value >> 16) & 0xFF);
          output.data.push_back((value >>  8) & 0xFF);
          output.data.push_back((value >>  0) & 0xFF);
        } else { return -1; }
      }
    }

    return 0;
  }
}
