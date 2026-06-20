#include <gtest/gtest.h>
#include "../src/main.cpp"

TEST(Main, FailOnInvalidArgumentLengts) {
	{
		const int ARGC = 4;
		const char* ARGV[ARGC] = { "algolab", "png", "encode", "data/test.bmp" };

		EXPECT_EQ(entry(ARGC, ARGV), -1);
	};

	{
		const int ARGC = 3;
		const char* ARGV[ARGC] = { "algolab", "png", "decode" };

		EXPECT_EQ(entry(ARGC, ARGV), -1);
	};
}

TEST(Main, FailOnInvalidTarget) {
	const int ARGC = 5;
	const char* ARGV[ARGC] = { "algolab", "jpg", "encode", "data/test.bmp", "out.jpg" };

	EXPECT_EQ(entry(ARGC, ARGV), -1);
}

TEST(Main, FailOnInvalidFeature) {
	const int ARGC = 5;
	const char* ARGV[ARGC] = { "algolab", "png", "reconstruct", "data/test.bmp", "out.png" };

	EXPECT_EQ(entry(ARGC, ARGV), -1);
}

TEST(BMP_decode, FailOnInvalidBMP) {
	const int ARGC = 4;
	const char* ARGV[ARGC] = { "algolab", "bmp", "decode", "data/fake.bmp" };

	EXPECT_EQ(entry(ARGC, ARGV), -1);
}

TEST(BMP_decode, SucceedOnValid1BPPBMP) {
	const int ARGC = 5;
	const char* ARGV[ARGC] = { "algolab", "bmp", "decode", "data/test.bmp", "out.png" };

	EXPECT_EQ(entry(ARGC, ARGV), 0);
}

TEST(BMP_decode, SucceedOnValid24BPPBMP) {
	const int ARGC = 5;
	const char* ARGV[ARGC] = { "algolab", "bmp", "decode", "data/test3.bmp", "out.png" };

	EXPECT_EQ(entry(ARGC, ARGV), 0);
}

TEST(PNG_encode, SucceedOnValidFilePath) {
	const int ARGC = 5;
	const char* ARGV[ARGC] = { "algolab", "png", "encode", "data/test.bmp", "out.png" };

	EXPECT_EQ(entry(ARGC, ARGV), 0);
}

TEST(QOI_encode, SucceedOnValidFilePath) {
	const int ARGC = 5;
	const char* ARGV[ARGC] = { "algolab", "qoi", "encode", "data/test.bmp", "out.png" };

	EXPECT_EQ(entry(ARGC, ARGV), 0);
}

TEST(PNG_encode, SucceedOnCubeBMP) {
	const int ARGC = 5;
	const char* ARGV[ARGC] = { "algolab", "png", "encode", "data/dice.bmp", "out.png" };

	EXPECT_EQ(entry(ARGC, ARGV), 0);
}

TEST(QOI_encode, SucceedOnCubeBMP) {
	const int ARGC = 5;
	const char* ARGV[ARGC] = { "algolab", "qoi", "encode", "data/dice.bmp", "out.png" };

	EXPECT_EQ(entry(ARGC, ARGV), 0);
}

TEST(PNG_encode_decode, CanProduceOriginal) {
	Image image = {};
	const char* source = "data/test.bmp";
	EXPECT_EQ(source_exists(source), 0);
	EXPECT_EQ(bmp::decode(source, image), 0);

	std::vector<uint8_t>png;
	EXPECT_GE(png::encode(image, png), 0);

	const char* out = "tests/test.png";
	std::ofstream pngOutFile(out, std::ios::binary);
	EXPECT_EQ(pngOutFile.is_open(), true);
	pngOutFile.write((const char *)png.data(), png.size());
	pngOutFile.close();
	EXPECT_EQ(source_exists(out), 0);

	Image output_image = {};
	EXPECT_EQ(png::decode(out, output_image), 0);

	EXPECT_EQ(image.height, output_image.height);
	EXPECT_EQ(image.width, output_image.width);

	int diff = 0;
	for (int i = 0; i < image.data.size(); ++i) {
		if (image.data[i] != output_image.data[i]) diff++;
		EXPECT_EQ(image.data[i], output_image.data[i]);
	}

	EXPECT_EQ(diff, 0);
}

TEST(QOI_encode_decode, CanProduceOriginal) {
  Image image = {};
  const char* source = "data/test.bmp";
  EXPECT_EQ(source_exists(source), 0);
	EXPECT_EQ(bmp::decode(source, image), 0);

  std::vector<uint8_t>qoi;
	EXPECT_GE(qoi::encode(image, qoi), 0);

	const char* out = "tests/test.qoi";
  std::ofstream qoiOutFile(out, std::ios::binary);
  EXPECT_EQ(qoiOutFile.is_open(), true);
  qoiOutFile.write((const char *)qoi.data(), qoi.size());
  qoiOutFile.close();
  EXPECT_EQ(source_exists(out), 0);

  Image output_image = {};
  EXPECT_EQ(qoi::decode(out, output_image), 0);

  EXPECT_EQ(image.height, output_image.height);
  EXPECT_EQ(image.width, output_image.width);

  int diff = 0;
  for (int i = 0; i < image.data.size(); ++i) {
  	if (image.data[i] != output_image.data[i]) diff++;
  	EXPECT_EQ(image.data[i], output_image.data[i]);
  }

  EXPECT_EQ(diff, 0);
}

#ifdef USE_GTEST_MAIN
int main(int argc, char** argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
#endif
