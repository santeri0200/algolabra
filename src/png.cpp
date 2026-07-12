#include <algorithm>
#include <cstdint>
#include <cmath>
#include <fstream>
#include <iostream>
#include <array>
#include <vector>
#include <cstring>
#include <zlib.h>

#include "image.cpp"
#include "deflate.cpp"

static uint32_t Paeth(uint32_t a, uint32_t b, uint32_t c) {
    uint32_t p = a + b - c;
    uint32_t pa = abs((int32_t)(p - a));
    uint32_t pb = abs((int32_t)(p - b));
    uint32_t pc = abs((int32_t)(p - c));

    if (pa <= pb && pa <= pc) {
      return a;
    }

    if (pb <= pc) {
      return b;
    }

    return c;
}

void pushChunk(
  std::vector<uint8_t>& png,
  const char type[4],
  const std::vector<uint8_t>& data
) {
  WriteBE32(png, (uint32_t)data.size());

  png.push_back(type[0]);
  png.push_back(type[1]);
  png.push_back(type[2]);
  png.push_back(type[3]);

  png.insert(png.end(), data.begin(), data.end());

  uint32_t crc = crc32(0, nullptr, 0);
  crc = crc32(crc, (const Bytef*)type, 4);
  crc = crc32(crc, data.data(), data.size());

  WriteBE32(png, crc);
}

namespace png {
  int encode(const Image& img, std::vector<uint8_t> &output) {
    uint32_t height = img.height;
    uint32_t width = img.width;
    uint32_t stride = width * 4;

    std::array<uint8_t, 8> signature{ 137, 80, 78, 71, 13, 10, 26, 10 };

    output.insert(output.end(), signature.begin(), signature.end());

    std::vector<uint8_t> ihdr; {
      WriteBE32(ihdr, width);
      WriteBE32(ihdr, height);

      ihdr.insert(ihdr.end(), {
        8, // bit depth
        6, // RGBA
        0, // compression
        0, // filter
        0, // interlace
      });

      pushChunk(output, "IHDR", ihdr);
    }

    std::vector<uint8_t> filtered;
    filtered.reserve(height * (stride + 1)); {
      for (uint32_t y = 0; y < height; ++y) {
        filtered.push_back(0); // Filter type 

        const uint8_t* row = img.data.data() + (y * stride);
        filtered.insert(filtered.end(), row, row + stride);
      }
    }

    std::vector<uint8_t> compressed;
    DeflateStored(
        filtered.data(),
        filtered.size(),
        compressed);

    pushChunk(output, "IDAT", compressed);
    pushChunk(output, "IEND", std::vector<uint8_t>{});

    return 0;
  }
}
