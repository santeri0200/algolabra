#include <cstring>
#include <filesystem>
#include <iostream>

#include "qoi.cpp"
#include "png.cpp"

int entry(int argc, char const *argv[]) {
  if (argc != 3) {
    std::cerr << "Invalid amount of arguments!\n";

    return -1;
  }

  const char *type = argv[1];
  const char *mode = argv[2];
  const char *source = argv[3];

  if (!std::filesystem::exists(source)) {
    return -1;
  }

  if (strcmp(type, "qoi") == 0 && strcmp(mode, "decode") == 0) {
    return qoi::decode(source);
  } else if (strcmp(type, "png") == 0 && strcmp(mode, "decode") == 0) {
    return png::decode(source);
  }

  return 0;
}
