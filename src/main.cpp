#include <cstring>
#include <string>
#include <filesystem>
#include <iostream>

#include "qoi.cpp"
#include "png.cpp"
#include "bmp.cpp"

struct Options {
  std::string mode;
  std::string input;
  std::string output;
};

int source_exists(const std::string& source) {
  return static_cast<int>(std::filesystem::exists(source)) - 1;
}

int output_folder_exists(const std::string& output) {
  bool exists = std::filesystem::exists(output);
  bool is_folder = std::filesystem::is_directory(output);

  return static_cast<int>(exists && is_folder) - 1;
}

int read_options(int argc, char** argv, Options &opts) {
  if (argc < 2) {
    std::cerr << "No mode set! Try running the program with \"--help\" flag enabled.\n";
    return -1;
  }

  std::string mode = argv[1];
  if (mode == "--help") {
    std::cerr << "Program usage:\n"
              << "\tmain [MODE] [INPUT] [OUTPUT]\n\n"
              << "\tValid [MODE]s are \"--encode\" and \"--decode\" instead. (Only one mode is allowed at a time!)\n"
              << "\t[INPUT] should be a BMP file containing \"raw\" data.\n"
              << "\t[OUTPUT] should be a folder the encoded file is stored in at.\n";
    return 0;
  }

  if (mode == "--decode") {
    opts.mode = "decode";
  } else if (mode == "--encode") {
    opts.mode = "encode";
  } else {
    std::cerr << "Invalid mode! Use \"--encode\" or \"--decode\" instead.\n";
    return -1;
  }

  if (argc < 3) {
    std::cerr << "No input set! Try running the program with \"--help\" flag enabled.\n";
    return -1;
  }

  std::string input = argv[2];
  opts.input = input;

  if (input.empty()) {
    std::cerr << "Invalid input file!\n";
    return -1;
  }

  if (
    input.size() < 4 ||
    (
      input.compare(input.size() - 4, 4, ".bmp") != 0 &&
      input.compare(input.size() - 4, 4, ".qoi") != 0 &&
      input.compare(input.size() - 4, 4, ".png") != 0
    )
  ) {
    std::cerr << "Unsupported file extension!\n";
    return -1;
  }

  if (argc > 3) {
    std::string output = argv[3];
    opts.output = output;

    if (output.empty()) {
      std::cerr << "Invalid output folder!\n";
      return -1;
    }
  }

  return 0;
}

int write_output(const std::string &output, std::vector<uint8_t> &data) {
    std::ofstream outFile(output, std::ios::binary);

    if (!outFile.is_open()) {
      std::cerr << "Failed to open " << output << "\n";
      return -1;
    }

    outFile.write((const char *)data.data(), data.size());
    outFile.close();

    return 0;
}

int decode(const std::string& source, Image &image) {
  std::ifstream file(source, std::ios::binary);
  if (!file || !file.is_open()) {
    std::cerr << "Error opening file!\n";

    return -1;
  }

  std::vector<uint8_t> input(
    (std::istreambuf_iterator<char>(file)),
    std::istreambuf_iterator<char>()
  );

  if (source.compare(source.size() - 4, 4, ".bmp") == 0) {
    file.close();
    return bmp::decode(input, image);
  }

  if (source.compare(source.size() - 4, 4, ".qoi") == 0) {
    file.close();
    return qoi::decode(input, image);
  }

  if (source.compare(source.size() - 4, 4, ".png") == 0) {
    file.close();
    return png::decode(input, image);
  }

  std::cerr << "Unsupported file extension!\n";
  return -1;
}

int entry(int argc, char** argv) {
  Options opts = {};
  opts.input = "";
  opts.output = "";

  if (read_options(argc, argv, opts) < 0) {
    return -1;
  }

  if (source_exists(opts.input) != 0) {
    std::cerr << opts.input << " does not exist!\n";
    return -1;
  }

  if (!opts.output.empty() && output_folder_exists(opts.output) != 0) {
    std::cerr << opts.output << " does not exist or is not a directory!\n";
    return -1;
  }

  if (opts.mode == "decode") {
    Image image = {};
    if (decode(opts.input, image) < 0) {
      std::cerr << "Failed to decode " << opts.input << "\n";
      return -1;
    }

    std::cout << opts.input << " decoded successfully!\n";
  }

  if (opts.mode == "encode") {
    Image image = {};
    if (decode(opts.input, image) < 0) {
      std::cerr << "Failed to decode the input file!\n";
      return -1;
    }

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

    size_t originalSize = std::filesystem::file_size(opts.input);
    size_t qoiSize = qoiOutput.size();
    size_t pngSize = pngOutput.size();

    size_t pixelSize = (size_t)image.height * (size_t)image.width * 4;

    std::cerr << "width: " << image.width << ", height: " << image.width << ", pixels: " << image.data.size() / 4 << '\n';
    std::cerr << "raw size: " << pixelSize << " bytes\n";
    std::cerr << "original size: " << originalSize << " bytes, compression ratio: " << (float)pixelSize / (float)originalSize << ":1\n";
    std::cerr << "qoi size: " << qoiSize << " bytes, compression ratio: " << (float)pixelSize / (float)qoiSize << ":1\n";
    std::cerr << "png size: " << pngSize << " bytes, compression ratio: " << (float)pixelSize / (float)pngSize << ":1\n";

    std::cerr << "\nqoi output is the " << (float)qoiSize / (float)pngSize * 100.0 << "% size of png output.\n";

    if (!opts.output.empty()) {
      if (write_output(opts.output + "/out.qoi", qoiOutput) < 0) {
        std::cerr << "Failed to write " << opts.output + "/out.qoi" << "\n";
        return -1;
      }

      if (write_output(opts.output + "/out.png", pngOutput) < 0) {
        std::cerr << "Failed to write " << opts.output + "/out.png" << "\n";
        return -1;
      }
    }
  }

  return 0;
}

#ifndef USE_GTEST_MAIN
int main(int argc, char** argv) {
  return entry(argc, argv);
}
#endif
