#include <algorithm>
#include <cstdint>
#include <cmath>
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
  int decode(const std::vector<uint8_t> &input, Image &output) {
    std::array<uint8_t, 8> signature{ 137, 80, 78, 71, 13, 10, 26, 10 };
    uint32_t width     = 0;
    uint32_t height    = 0;
    uint8_t  bitDepth  = 0;
    uint8_t  colorType = 0;

    if (input.size() < signature.size() || !std::equal(signature.begin(), signature.end(), input.begin())) {
      return -1;
    }

    bool seenIEND = false;
    bool seenIHDR = false;

    size_t pos = 8;
    std::vector<uint8_t> idat; // Image data
    while (pos + 12 <= input.size()) {
      uint32_t length = ReadBE32(&input[pos]);
      pos += 4;

      if (input.size() < pos + 4) {
        return -1;
      }

      uint32_t type = ReadBE32(&input[pos]);
      pos += 4;

      size_t chunkEnd = pos + length + 4;
      if (input.size() < chunkEnd) {
        return -1;
      }

      if (type == 0x49484452) {
        seenIHDR = true;

        width     = ReadBE32(&input[pos + 0]);
        height    = ReadBE32(&input[pos + 4]);
        bitDepth  = input[pos + 8];
        colorType = input[pos + 9];

        if (bitDepth != 8) { return -1; }
        // 2: RGB
        // 6: RGBA
        if (colorType != 2 && colorType != 6) { return -1; }
      } else if (type == 0x49444154) {
        // IDAT
        if (!seenIHDR) {
          return -1;
        }

        idat.insert(
          idat.end(),
          input.begin() + pos,
          input.begin() + pos + length
        );
      } else if (type == 0x49454E44) {
        seenIEND = true;
        if (length != 0) {
          return -1;
        }

        pos += length; // zero
        pos += 4;      // CRC
        break;
      } else {
        return -1;
      }

      pos += length;
      pos += 4;
    }

    if (!seenIEND || !seenIHDR) {
      return -1;
    }

    uint32_t channels = (colorType == 6) ? 4 : 3;
    uint32_t stride = width * channels;
    std::vector<uint8_t> inflatedData;

    if (InflateStored(idat, inflatedData) != 0) {
        return -1337;
    }

    size_t inflatedSize = inflatedData.size();


    size_t expectedSize = static_cast<size_t>(stride + 1) * height;
    if (inflatedSize != expectedSize) {
      return -2;
    }

    std::vector<uint8_t> raw(height * stride);
    for (uint32_t y = 0; y < height; ++y) {
      uint8_t* out = raw.data() + (y * stride);
      uint8_t* scan = inflatedData.data() + (y * (stride + 1));

      uint8_t filter = scan[0];
      scan++;

      for (uint32_t x = 0; x < stride; ++x) {
        uint8_t left   = (channels <= x) ? out[x - channels] : 0;
        uint8_t up     = (0 < y)         ? raw[((y - 1) * stride) + x] : 0;
        uint8_t upLeft = (channels <= x && 0 < y) ? raw[((y - 1) * stride) + x - channels] : 0;

        out[x] = scan[x];
        switch (filter) {
          case 0:
            break;
          case 1:
            out[x] += left;
            break;
          case 2:
            out[x] += up;
            break;
          case 3:
            out[x] += (left + up) >> 1;
            break;
          case 4:
            out[x] += Paeth(left, up, upLeft);
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
        output.data[j++] = raw[(i * 3) + 0];
        output.data[j++] = raw[(i * 3) + 1];
        output.data[j++] = raw[(i * 3) + 2];
        output.data[j++] = 255;
      }
    }

    // Raw is the rgba buffer, so it has to be cast to rgb if colorType is 2.
    return 0;
  }

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
