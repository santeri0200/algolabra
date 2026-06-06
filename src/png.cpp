#include <algorithm>
#include <cstdint>
#include <cmath>
#include <fstream>
#include <iostream>
#include <array>
#include <vector>

#include "image.cpp"

#include <zlib.h>

namespace png {
  static uint32_t Paeth(uint32_t a, uint32_t b, uint32_t c) {
      uint32_t p = a + b - c;
      uint32_t pa = abs((int32_t)(p - a));
      uint32_t pb = abs((int32_t)(p - b));
      uint32_t pc = abs((int32_t)(p - c));

      if (pa <= pb && pa <= pc) return a;
      if (pb <= pc) return b;
      return c;
  }

  int decode(const char *source) {
    std::array<uint8_t, 8> signature{ 137, 80, 78, 71, 13, 10, 26, 10 };
    uint32_t width     = 0;
    uint32_t height    = 0;
    uint8_t  bitDepth  = 0;
    uint8_t  colorType = 0;

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

    if (data.size() < signature.size() || !std::equal(signature.begin(), signature.end(), data.begin())) {
      return -1;
    }

    uint8_t pos = 8;
    std::vector<uint8_t> idat; // Image data
    while (pos + 12 <= data.size()) {
      uint32_t length = *(uint32_t*)(&data[pos]);
      pos += 4;

      uint32_t type = *(uint32_t*)(&data[pos]);
      pos += 4;

      if (data.size() < pos + length + 4) {
        return -1;
      }

      if (type == 0x49484452) {
        // IHDR
        width     = *(uint32_t*)(&data[pos + 0]);
        height    = *(uint32_t*)(&data[pos + 4]);
        bitDepth  = data[pos + 8];
        colorType = data[pos + 9];

        if (bitDepth != 8) { return -1; }
        // 2: RGB
        // 6: RGBA
        if (colorType != 2 && colorType != 6) { return -1; }
      } else if (type == 0x49444154) {
        // IDAT
      } else if (type == 0x49454E44) {
        // IEND
      } else {
        return -1;
      }

      pos += length;
      pos += 4;
    }

    uint32_t channels = (colorType == 6) ? 4 : 3;
    uint32_t stride = width * channels;
    uint64_t inflatedSize = (uint64_t)(stride + 1) * height;

    std::vector<uint8_t> inflatedData(inflatedSize);
    if (uncompress(inflatedData.data(), &inflatedSize, idat.data(), idat.size()) != Z_OK) {
      return -1;
    }

    std::vector<uint8_t> raw(height * stride);
    for (uint32_t y = 0; y < height; ++y) {
      uint8_t* out = raw.data() + y * stride;
      uint8_t* scan = inflatedData.data() + y * (stride + 1);

      uint8_t filter = scan[0];
      scan++;

      for (uint32_t x = 0; x < stride; ++x) {
        uint8_t left   = (channels <= x) ? out[x - channels] : 0;
        uint8_t up     = (0 < y)         ? raw[(y - 1) * stride + x] : 0;
        uint8_t upLeft = (channels <= x && 0 < y) ? raw[(y - 1) * stride + x - channels] : 0;

        uint8_t v = scan[x];
        switch (filter) {
          case 0:
            out[x] = v;
            break;
          case 1:
            out[x] = (v + left);
            break;
          case 2:
            out[x] = (v + up);
            break;
          case 3:
            out[x] = (v + ((left + up) >> 1));
            break;
          case 4:
            out[x] = (v + Paeth(left, up, upLeft));
            break;

          default:
            return -1;
        }
      }
    }

    // Raw is the rgba buffer, so it has to be cast to rgb if colorType is 2.
    return 0;
  }

  __inline__ void WriteBE32(std::vector<uint8_t>& out, uint32_t x) {
    out.push_back((x >> 24) & 0xFF);
    out.push_back((x >> 16) & 0xFF);
    out.push_back((x >>  8) & 0xFF);
    out.push_back((x >>  0) & 0xFF);
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

    uint32_t crc = crc32(
      crc32(0, nullptr, 0),
      &png[png.size() - 4],
      4 + data.size()
    );

    WriteBE32(png, crc);
  }

  int encode(Image img, uint32_t type, uint8_t *output) {

    std::vector<uint8_t> data = img.data;
    uint32_t width = img.width;
    uint32_t height = img.height;

    std::array<uint8_t, 8> signature{ 137, 80, 78, 71, 13, 10, 26, 10 };
    std::vector<uint8_t> png;
    uint32_t lenght = width * height;

    png.insert(png.end(), signature.begin(), signature.end());

    std::vector<uint8_t> ihdr; {
      WriteBE32(ihdr, width);
      WriteBE32(ihdr, height);

      ihdr.push_back(8); // bit depth
      ihdr.push_back(6); // RGBA
      ihdr.push_back(0); // compression
      ihdr.push_back(0); // filter
      ihdr.push_back(0); // interlace

      pushChunk(png, "IHDR", ihdr);
    }

    std::vector<uint8_t> filtered(height * (width * 4 + 1)); {
      uint32_t stride = width * 4;
      for (uint32_t y = 0; y < height; ++y) {
        filtered.push_back(0);

        const uint8_t* row = data.data() + y * stride;
        filtered.insert(filtered.end(), row, row + stride);
      }
    }

    uint64_t compressedSize = compressBound(filtered.size());
    std::vector<uint8_t> compressed(compressedSize);

    int32_t rc = compress2(
      compressed.data(),
      &compressedSize,
      filtered.data(),
      filtered.size(),
      Z_BEST_COMPRESSION
    );

    if (rc != Z_OK) {
      return -1;
    }

    compressed.resize(compressedSize);
    pushChunk(png, "IDAT", compressed); {
      std::vector<uint8_t> empty;
      pushChunk(png, "IEND", empty);
    }

    return 0;
  }
}
