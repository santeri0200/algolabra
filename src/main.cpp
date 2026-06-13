#include <cstring>
#include <filesystem>
#include <iostream>

#include "qoi.cpp"
#include "png.cpp"
#include "bmp.cpp"

int entry(int argc, char const *argv[]) {
  if (argc < 4) {
    std::cerr << "Invalid amount of arguments!" << argc << "\n";

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
  } else if (strcmp(mode, "encode") == 0) {
    if (argc < 5) {
      std::cerr << "Invalid amount of arguments!" << argc << "\n";

      return -1;
    }

    const char *dist = argv[4];

    Image image = {};
    if (bmp::decode(source, image) < 0) {
      std::cerr << "Failed to process the bmp file!\n";
      return -1;
    }

    std::vector<uint8_t>output;
    if (strcmp(type, "qoi") == 0) {
      if (qoi::encode(image, output) < 0) {
        std::cerr << "Failed to encode!\n";
        return -1;
      }
    } else if (strcmp(type, "png") == 0) {
      if (png::encode(image, output) < 0) {
        std::cerr << "Failed to encode!\n";
        return -1;
      }
    }

    std::ofstream outFile(dist, std::ios::binary);
    if (outFile.is_open()) {
        outFile.write((const char *)output.data(), output.size());
        outFile.close();
    } else {
      std::cerr << "Failed to write the file\n";
    }
  }

  return 0;
}

#ifndef USE_GTEST_MAIN
int main(int argc, char const *argv[]) {
  entry(argc, argv);
}
#endif
