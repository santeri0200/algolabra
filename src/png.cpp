#include <algorithm>
#include <cstdint>
#include <cmath>
#include <fstream>
#include <iostream>
#include <array>
#include <vector>
#include <cstring>

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

  uint32_t ReadBE32(const uint8_t* p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8)  | uint32_t(p[3]);
  }

  int decode(const char *source, Image &output) {
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

    bool seenIEND = false;
    bool seenIHDR = false;

    size_t pos = 8;
    std::vector<uint8_t> idat; // Image data
    while (pos + 12 <= data.size()) {
      uint32_t length = ReadBE32(&data[pos]);
      pos += 4;

      if (data.size() < pos + 4) return -1;
      uint32_t type = ReadBE32(&data[pos]);
      pos += 4;

      size_t chunkEnd = pos + length + 4;
      if (data.size() < chunkEnd) return -1;

      if (type == 0x49484452) {
        seenIHDR = true;

        width     = ReadBE32(&data[pos + 0]);
        height    = ReadBE32(&data[pos + 4]);
        bitDepth  = data[pos + 8];
        colorType = data[pos + 9];

        if (bitDepth != 8) { return -1; }
        // 2: RGB
        // 6: RGBA
        if (colorType != 2 && colorType != 6) { return -1; }
      } else if (type == 0x49444154) {
        // IDAT
        if (!seenIHDR) return -1;
        idat.insert(
          idat.end(),
          data.begin() + pos,
          data.begin() + pos + length
        );
      } else if (type == 0x49454E44) {
        seenIEND = true;
        if (length != 0) return -1;

        pos += length; // zero
        pos += 4;      // CRC
        break;
      } else {
        return -1;
      }

      pos += length;
      pos += 4;
    }

    if (!seenIEND || !seenIHDR) return -1;

    uint32_t channels = (colorType == 6) ? 4 : 3;
    uint32_t stride = width * channels;
    uLongf inflatedSize = (uLongf)(stride + 1ULL) * height;

    std::vector<uint8_t> inflatedData(inflatedSize);
    if (uncompress(inflatedData.data(), &inflatedSize, idat.data(), idat.size()) != Z_OK) {
      return -1337;
    }

    size_t expectedSize = static_cast<size_t>(stride + 1) * height;
    if (inflatedSize != expectedSize) return -2;

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
            out[x] = v + left;
            break;
          case 2:
            out[x] = v + up;
            break;
          case 3:
            out[x] = v + ((left + up) >> 1);
            break;
          case 4:
            out[x] = v + Paeth(left, up, upLeft);
            break;

          default:
            return -1;
        }
      }
    }

    output.width = width;
    output.height = height;

    output.data.resize(width * height * 4);
    if (colorType == 6) {
      std::memcpy(output.data.data(), raw.data(), raw.size());
    } else {
      size_t j = 0;
      for (size_t i = 0; i < width * height; ++i) {
        output.data[j++] = raw[i * 3 + 0];
        output.data[j++] = raw[i * 3 + 1];
        output.data[j++] = raw[i * 3 + 2];
        output.data[j++] = 255;
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

    uint32_t crc = crc32(0, nullptr, 0);
    crc = crc32(crc, (const uint8_t*)type, 4);
    crc = crc32(crc, data.data(), data.size());

    WriteBE32(png, crc);
  }

  int encode(Image img, std::vector<uint8_t> &output) {
    std::vector<uint8_t> data = img.data;
    uint32_t width = img.width;
    uint32_t height = img.height;

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
    filtered.reserve(height * (width * 4 + 1)); {
      uint32_t stride = width * 4;
      for (uint32_t y = 0; y < height; ++y) {
        filtered.push_back(0);

        const uint8_t* row = data.data() + y * stride;
        filtered.insert(filtered.end(), row, row + stride);
      }
    }

    uint64_t compressedSize = compressBound(filtered.size());
    uLongf zDestLen = static_cast<uLongf>(compressedSize);
    std::vector<uint8_t> compressed(compressedSize);

    int32_t rc = compress2(
      compressed.data(),
      &zDestLen,
      filtered.data(),
      filtered.size(),
      Z_BEST_COMPRESSION
    );

    if (rc != Z_OK) {
      return -1;
    }

    compressed.resize(compressedSize);
    pushChunk(output, "IDAT", compressed);
    pushChunk(output, "IEND", std::vector<uint8_t>{});

    return 0;
  }
}
