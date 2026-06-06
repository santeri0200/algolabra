#pragma once
#include <cstdint>
#include <vector>

struct Image {
  uint32_t width;
  uint32_t height;
  std::vector<uint8_t> data;
};
