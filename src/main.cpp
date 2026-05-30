#include <cstring>
#include <filesystem>
#include <iostream>

#include "qoi.cpp"

int entry(int argc, char const* argv[]) {
  if (argc != 2) {
    std::cerr << "Invalid amount of arguments!\n";

    return -1;
  }

  const char* mode = argv[1];
  const char* source = argv[2];

  if (!std::filesystem::exists(source)) {
    return -1;
  }

  if (strcmp(mode, "decode") == 0) {
    return decode(source);
  }

  return 0;
}
