#include <cstdint>
#include <cstring>
#include <iostream>
#include <fstream>

struct QOIHeaders {
  char     magic[4];
  uint32_t width;
  uint32_t height;
  uint8_t  channels;
  uint8_t  colorspace;
};

union Headers {
  QOIHeaders structure;
  uint8_t    data[14];
};

__inline__ int chech_header_validity(Headers headers) {
  char magic[4] = {'q', 'o', 'i', 'f'};

  // Magic does not match
  if ((uint32_t)(headers.data[0]) != (uint32_t)(magic[0])) {
    return -1;
  }

  return 0;
}

__inline__ uint8_t get_position_index(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  return (r * 3 + g * 5 + b * 7 + a * 11) % 64;
}

__inline__ int8_t cast_i2_to_i8(uint8_t x) {
  return (int8_t)(x << 6) >> 6;
}

__inline__ int8_t cast_i4_to_i8(uint8_t x) {
  return (int8_t)(x << 4) >> 4;
}

__inline__ int8_t cast_i6_to_i8(uint8_t x) {
  return (int8_t)(x << 2) >> 2;
}

struct Color {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
};

union ColorData {
  Color    color;
  uint32_t data;
};

int decode(int argc, char const* argv[]) {
  uint8_t raw_headers[14] = {};
  uint32_t colors[64] = {};
  ColorData current_color = {.data = 0x000000FF}; // Current color is defined to start with rgb of 0 and alpha of 1.

  if (argc != 2) {
    std::cerr << "Invalid amount of arguments!\n";

    return -1;
  }

  // Currently requires there to be one commandline argument (the filename)
  std::ifstream file(argv[1], std::ios::binary);
  if (!file || !file.is_open()) {
    std::cerr << "Error opening file!\n";

    return -1;
  }

  for (auto slot : raw_headers) file.read((char*) &slot, 1);

  Headers headers;
  memcpy(headers.data, raw_headers, sizeof headers.data);
  if (!chech_header_validity(headers)) {
    return -1;
  }

  uint8_t data;
  while (file.read((char*) &data, sizeof data)) {
    switch (data) {
      case 0b00000000 ... 0b00000000: // OP_INDEX
        current_color.data = colors[data & 0b00111111];
        break;
      case 0b01000000 ... 0b01000000: // OP_DIFF
        current_color.color.r = (uint8_t)(current_color.color.r + cast_i2_to_i8((data & 0b00110000) >> 4));
        current_color.color.g = (uint8_t)(current_color.color.g + cast_i2_to_i8((data & 0b00001100) >> 2));
        current_color.color.b = (uint8_t)(current_color.color.b + cast_i2_to_i8((data & 0b00000011) >> 0));
        break;
      case 0b10000000 ... 0b10000000: // OP_LUMA
        data = cast_i6_to_i8(data & 0b00111111);
        current_color.color.r = (uint8_t)(current_color.color.r + data);
        current_color.color.g = (uint8_t)(current_color.color.g + data);
        current_color.color.b = (uint8_t)(current_color.color.b + data);

        // Failed to read second LUMA byte
        if (!file.read((char*) &data, sizeof data)) { return -1; }
        current_color.color.r = (uint8_t)(current_color.color.r - cast_i4_to_i8((data & 0b11110000) >> 4));
        current_color.color.b = (uint8_t)(current_color.color.b - cast_i4_to_i8((data & 0b00001111) >> 0));
        break;
      case 0b11000000 ... 0b11111100: // TODO: OP_RUN

        break;
      case 0b11111110: // OP_RGB
        // Failed to read first RGB byte
        if (!file.read((char*) &data, sizeof data)) { return -1; }
        current_color.color.r = data;

        // Failed to read second RGB byte
        if (!file.read((char*) &data, sizeof data)) { return -1; }
        current_color.color.g = data;

        // Failed to read third RGB byte
        if (!file.read((char*) &data, sizeof data)) { return -1; }
        current_color.color.b = data;

        break;
      case 0b11111111: // OP_RGBA
        // Failed to read first RGBA byte
        if (!file.read((char*) &data, sizeof data)) { return -1; }
        current_color.color.r = data;

        // Failed to read second RGBA byte
        if (!file.read((char*) &data, sizeof data)) { return -1; }
        current_color.color.g = data;

        // Failed to read third RGBA byte
        if (!file.read((char*) &data, sizeof data)) { return -1; }
        current_color.color.b = data;

        // Failed to read fourth RGBA byte
        if (!file.read((char*) &data, sizeof data)) { return -1; }
        current_color.color.a = data;

        break;
    }
  }

  file.close();
  
  return 0;
}
