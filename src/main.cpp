#include <cstring>
#include <filesystem>
#include <iostream>

#include "qoi.cpp"
#include "png.cpp"
#include "bmp.cpp"

int source_exists(const char *source) {
  if (!std::filesystem::exists(source)) {
    return -1;
  }

  return 0;
}

int entry(int argc, char const *argv[]) {
  if (argc < 4) {
    std::cerr << "Invalid amount of arguments!" << argc << "\n";

    return -1;
  }

  const char *type = argv[1];
  const char *mode = argv[2];
  const char *source = argv[3];

  if (source_exists(source) != 0) {
    return -1;
  }

  if (strcmp(mode, "decode") == 0) {
    Image image = {};

    if (strcmp(type, "qoi") == 0) {
      return qoi::decode(source, image);
    } else if (strcmp(type, "png") == 0) {
      return png::decode(source, image);
    } else if (strcmp(type, "bmp") == 0) {
      return bmp::decode(source, image);
    }
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
    } else {
      return -1;
    }

    std::ofstream outFile(dist, std::ios::binary);
    if (outFile.is_open()) {
        outFile.write((const char *)output.data(), output.size());
        outFile.close();
    } else {
      std::cerr << "Failed to write the file\n";
    }

    return 0;
  } else  if (strcmp(mode, "compare") == 0) {
    Image image = {};
    if (bmp::decode(source, image) < 0) {
      std::cerr << "Failed to process the bmp file!\n";
      return -1;
    }

    if (strcmp(type, "both") == 0) {
      std::vector<uint8_t>qoiOutput;
      if (qoi::encode(image, qoiOutput) < 0) {
        std::cerr << "Failed to encode qoi!\n";
        return -1;
      }

      std::vector<uint8_t>pngOutput;
      if (png::encode(image, pngOutput) < 0) {
        std::cerr << "Failed to encode png!\n";
        return -1;
      }

      size_t originalSize = std::filesystem::file_size(source);
      size_t qoiSize = qoiOutput.size();
      size_t pngSize = pngOutput.size();

      std::cout << "width: " << image.width << ", height: " << image.width << ", pixels: " << image.data.size() / 4 << '\n';
      std::cout << "original size: " << originalSize << " bytes\n";
      std::cout << "qoi size: " << qoiSize << " bytes, compression ratio: " << (float)originalSize / (float)qoiSize << ":1\n";
      std::cout << "png size: " << pngSize << " bytes, compression ratio: " << (float)originalSize / (float)pngSize << ":1\n";

      std::cout << "\nqoi output is the " << (float)qoiSize / (float)pngSize * 100.0 << "% size of png output.\n";

      return 0;
    } else if (strcmp(type, "qoi") == 0) {
      std::vector<uint8_t>qoiOutput;
      if (qoi::encode(image, qoiOutput) < 0) {
        std::cerr << "Failed to encode qoi!\n";
        return -1;
      }

      size_t originalSize = std::filesystem::file_size(source);
      size_t qoiSize = qoiOutput.size();

      std::cout << "width: " << image.width << ", height: " << image.width << ", pixels: " << image.data.size() / 4 << '\n';
      std::cout << "original size: " << originalSize << " bytes\n";
      std::cout << "qoi size: " << qoiSize << " bytes, compression ratio: " << (float)originalSize / (float)qoiSize << ":1\n";

      return 0;
    } else if (strcmp(type, "png") == 0) {
      std::vector<uint8_t>pngOutput;
      if (png::encode(image, pngOutput) < 0) {
        std::cerr << "Failed to encode png!\n";
        return -1;
      }

      size_t originalSize = std::filesystem::file_size(source);
      size_t pngSize = pngOutput.size();

      std::cout << "width: " << image.width << ", height: " << image.width << ", pixels: " << image.data.size() / 4 << '\n';
      std::cout << "original size: " << originalSize << " bytes\n";
      std::cout << "png size: " << pngSize << " bytes, compression ratio: " << (float)originalSize / (float)pngSize << ":1\n";

      return 0;
    }
  }

  return -1;
}

#ifndef USE_GTEST_MAIN
int main(int argc, char const *argv[]) {
  entry(argc, argv);
}
#endif
