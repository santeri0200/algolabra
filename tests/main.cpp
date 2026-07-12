#include <gtest/gtest.h>
#include <vector>
#include "../src/main.cpp"

TEST(QOI, get_position_index) {
	Color a = { .r = 1, .g = 2, .b = 3, .a = 4 };
	EXPECT_EQ(get_position_index(a), 0b00001110);

	Color b = { .r = 2, .g = 3, .b = 4, .a = 5 };
	EXPECT_EQ(get_position_index(b), 0b00101000);

	Color c = { .r = 3, .g = 4, .b = 5, .a = 6 };
	EXPECT_EQ(get_position_index(c), 0b00000010);
}

TEST(QOI, chech_header_validity) {
	Headers h = {};
	h.structure.magic = { 'q', 'o', 'i', 'f' };
	EXPECT_EQ(qoi::chech_header_validity(h), 0);

	Headers d = {};
	d.structure.magic = { 'q', 'o', 'i', 'q' };
	EXPECT_EQ(qoi::chech_header_validity(d), -1);
}

TEST(QOI, decode_RGBA) {
	std::vector<uint8_t> data = {
		'q', 'o', 'i', 'f', // magic
		0, 0, 0, 1, // width
		0, 0, 0, 1, // height
		4, // channels
		0, // colorspace
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // RGBA white
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1
	};

	Image image = {};
	EXPECT_EQ(qoi::decode(data, image), 0);
	EXPECT_EQ(image.width, 1);
	EXPECT_EQ(image.height, 1);
	EXPECT_EQ(image.data.size(), 4);
	EXPECT_EQ(image.data.at(0), 0xFF);
	EXPECT_EQ(image.data.at(1), 0xFF);
	EXPECT_EQ(image.data.at(2), 0xFF);
	EXPECT_EQ(image.data.at(3), 0xFF);
}

TEST(QOI, encode_RGBA) {
	std::vector<uint8_t> test = {
		'q', 'o', 'i', 'f', // magic
		0, 0, 0, 1, // width
		0, 0, 0, 1, // height
		4, // channels
		0, // colorspace
		0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1
	};

	Image image = {};
	image.width = 1;
	image.height = 1;
	image.data = { 0xFF, 0xFF, 0xFF, 0xFE };

	std::vector<uint8_t> output = {};
	EXPECT_EQ(qoi::encode(image, output), 0);
	EXPECT_EQ(output.size(), test.size());
	for (size_t i = 0; i < output.size(); ++i) {
		EXPECT_EQ(output.at(i), test.at(i));
	}
}

TEST(QOI, decode_RGB) {
	std::vector<uint8_t> data = {
		'q', 'o', 'i', 'f', // magic
		0, 0, 0, 1, // width
		0, 0, 0, 1, // height
		4, // channels
		0, // colorspace
		0xFE, 0xFF, 0xFF, 0xFF, // RGB white
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1
	};

	Image image = {};
	EXPECT_EQ(qoi::decode(data, image), 0);
	EXPECT_EQ(image.width, 1);
	EXPECT_EQ(image.height, 1);
	EXPECT_EQ(image.data.size(), 4);
	EXPECT_EQ(image.data.at(0), 0xFF);
	EXPECT_EQ(image.data.at(1), 0xFF);
	EXPECT_EQ(image.data.at(2), 0xFF);
	EXPECT_EQ(image.data.at(3), 0xFF);
}

TEST(QOI, encode_RGB) {
	std::vector<uint8_t> test = {
		'q', 'o', 'i', 'f', // magic
		0, 0, 0, 1, // width
		0, 0, 0, 1, // height
		4, // channels
		0, // colorspace
		0xFE, 0x10, 0x40, 0xA0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1
	};

	Image image = {};
	image.width = 1;
	image.height = 1;
	image.data = { 0x10, 0x40, 0xA0, 0xFF };

	std::vector<uint8_t> output = {};
	EXPECT_EQ(qoi::encode(image, output), 0);
	EXPECT_EQ(output.size(), test.size());
	for (size_t i = 0; i < output.size(); ++i) {
		EXPECT_EQ(output.at(i), test.at(i));
	}
}

TEST(QOI, decode_DIFF) {
	std::vector<uint8_t> data = {
		'q', 'o', 'i', 'f', // magic
		0, 0, 0, 1, // width
		0, 0, 0, 1, // height
		4, // channels
		0, // colorspace
		0b01010101, // DIFF 
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1
	};

	Image image = {};
	EXPECT_EQ(qoi::decode(data, image), 0);
	EXPECT_EQ(image.width, 1);
	EXPECT_EQ(image.height, 1);
	EXPECT_EQ(image.data.size(), 4);
	EXPECT_EQ(image.data.at(0), 0xFF);
	EXPECT_EQ(image.data.at(1), 0xFF);
	EXPECT_EQ(image.data.at(2), 0xFF);
	EXPECT_EQ(image.data.at(3), 0xFF);
}

TEST(QOI, encode_DIFF) {
	std::vector<uint8_t> test = {
		'q', 'o', 'i', 'f', // magic
		0, 0, 0, 1, // width
		0, 0, 0, 1, // height
		4, // channels
		0, // colorspace
		0b01111111,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1
	};

	Image image = {};
	image.width = 1;
	image.height = 1;
	image.data = { 0x01, 0x01, 0x01, 0xFF };

	std::vector<uint8_t> output = {};
	EXPECT_EQ(qoi::encode(image, output), 0);
	EXPECT_EQ(output.size(), test.size());
	for (size_t i = 0; i < output.size(); ++i) {
		EXPECT_EQ(output.at(i), test.at(i));
	}
}

TEST(QOI, decode_LUMA) {
	std::vector<uint8_t> data = {
		'q', 'o', 'i', 'f', // magic
		0, 0, 0, 1, // width
		0, 0, 0, 1, // height
		4, // channels
		0, // colorspace
		0x9E, 0b01111001, // LUMA
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1
	};

	Image image = {};
	EXPECT_EQ(qoi::decode(data, image), 0);
	EXPECT_EQ(image.width, 1);
	EXPECT_EQ(image.height, 1);
	EXPECT_EQ(image.data.size(), 4);
	EXPECT_EQ(image.data.at(0), 0xFD);
	EXPECT_EQ(image.data.at(1), 0xFE);
	EXPECT_EQ(image.data.at(2), 0xFF);
	EXPECT_EQ(image.data.at(3), 0xFF);
}

TEST(QOI, encode_LUMA) {
	std::vector<uint8_t> test = {
		'q', 'o', 'i', 'f', // magic
		0, 0, 0, 1, // width
		0, 0, 0, 1, // height
		4, // channels
		0, // colorspace
		0x9E, 0b01111001, // LUMA
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1
	};

	Image image = {};
	image.width = 1;
	image.height = 1;
	image.data = { 0xFD, 0xFE, 0xFF, 0xFF };

	std::vector<uint8_t> output = {};
	EXPECT_EQ(qoi::encode(image, output), 0);
	EXPECT_EQ(output.size(), test.size());
	for (size_t i = 0; i < output.size(); ++i) {
		EXPECT_EQ(output.at(i), test.at(i));
	}
}

TEST(QOI, decode_RUN) {
	std::vector<uint8_t> data = {
		'q', 'o', 'i', 'f', // magic
		0, 0, 0, 2, // width
		0, 0, 0, 2, // height
		4, // channels
		0, // colorspace
		0xC3, // RUN
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1
	};

	Image image = {};
	EXPECT_EQ(qoi::decode(data, image), 0);
	EXPECT_EQ(image.width, 2);
	EXPECT_EQ(image.height, 2);
	EXPECT_EQ(image.data.size(), 16);
	EXPECT_EQ(image.data.at(0), 0x00);
	EXPECT_EQ(image.data.at(1), 0x00);
	EXPECT_EQ(image.data.at(2), 0x00);
	EXPECT_EQ(image.data.at(3), 0xFF);
	EXPECT_EQ(image.data.at(4), 0x00);
	EXPECT_EQ(image.data.at(5), 0x00);
	EXPECT_EQ(image.data.at(6), 0x00);
	EXPECT_EQ(image.data.at(7), 0xFF);
	EXPECT_EQ(image.data.at(8), 0x00);
	EXPECT_EQ(image.data.at(9), 0x00);
	EXPECT_EQ(image.data.at(10), 0x00);
	EXPECT_EQ(image.data.at(11), 0xFF);
	EXPECT_EQ(image.data.at(12), 0x00);
	EXPECT_EQ(image.data.at(13), 0x00);
	EXPECT_EQ(image.data.at(14), 0x00);
	EXPECT_EQ(image.data.at(15), 0xFF);
}

TEST(QOI, encode_RUN) {
	std::vector<uint8_t> test = {
		'q', 'o', 'i', 'f', // magic
		0, 0, 0, 2, // width
		0, 0, 0, 2, // height
		4, // channels
		0, // colorspace
		0xC3, // RUN
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1
	};

	Image image = {};
	image.width = 2;
	image.height = 2;
	image.data = {
		0x00, 0x00, 0x00, 0xFF,
		0x00, 0x00, 0x00, 0xFF,
		0x00, 0x00, 0x00, 0xFF,
		0x00, 0x00, 0x00, 0xFF,
	};

	std::vector<uint8_t> output = {};
	EXPECT_EQ(qoi::encode(image, output), 0);
	EXPECT_EQ(output.size(), test.size());
	for (size_t i = 0; i < output.size(); ++i) {
		EXPECT_EQ(output.at(i), test.at(i));
	}
}

// TEST(Main, FailOnInvalidFeature) {
// 	const int ARGC = 4;
// 	char* ARGV[ARGC] = { "algolab", "--reconstruct", "data/test.bmp", "out" };

// 	EXPECT_EQ(entry(ARGC, ARGV), -1);
// }

TEST(DEFLATE, BitWriter_Setup) {
	std::vector<uint8_t> v = { 1, 2, 3, 4 };
	BitWriter bw(v);

	EXPECT_EQ(bw.out, v);
	EXPECT_EQ(bw.buffer, 0);
	EXPECT_EQ(bw.bits, 0);
}

TEST(DEFLATE, BitWrite_Methods) {
	std::vector<uint8_t> v = { 1, 2, 3, 4 };
	BitWriter bw(v);

	bw.Write(0b11, 2);
	EXPECT_EQ(bw.out.size(), 4);
	EXPECT_EQ(bw.buffer, 3);
	EXPECT_EQ(bw.bits, 2);

	bw.Write(0b11, 2);
	EXPECT_EQ(bw.out.size(), 4);
	EXPECT_EQ(bw.buffer, 15);
	EXPECT_EQ(bw.bits, 4);

	bw.Write(0b11, 2);
	EXPECT_EQ(bw.out.size(), 4);
	EXPECT_EQ(bw.buffer, 63);
	EXPECT_EQ(bw.bits, 6);

	bw.Write(0b11, 2);
	EXPECT_EQ(bw.out.size(), 5);
	EXPECT_EQ(bw.buffer, 0);
	EXPECT_EQ(bw.bits, 0);

	EXPECT_EQ(bw.out.back(), 255);

	bw.Write(0b11, 2);
	EXPECT_EQ(bw.out.size(), 5);
	EXPECT_EQ(bw.buffer, 3);
	EXPECT_EQ(bw.bits, 2);

	bw.Flush();
	EXPECT_EQ(bw.out.size(), 6);
	EXPECT_EQ(bw.buffer, 0);
	EXPECT_EQ(bw.bits, 0);

	EXPECT_EQ(bw.out.back(), 3);
}

TEST(DEFLATE, ReverseBits) {
	uint16_t a = 0b1100;
	uint16_t b = 4;
	EXPECT_EQ(ReverseBits(a, b), 0b0011);

	uint16_t c = 0b1100;
	uint16_t d = 3;
	EXPECT_EQ(ReverseBits(c, d), 0b0001);
}

TEST(DEFLATE, LZ77_Setup) {
	size_t size = 1;
	LZ77 lz(size);

	EXPECT_EQ(lz.head.size(), lz.HASH);
	EXPECT_EQ(lz.head.at(0), -1);
	EXPECT_EQ(lz.head.at(lz.HASH - 1), -1);

	EXPECT_EQ(lz.prev.size(), size);
	EXPECT_EQ(lz.prev.at(0), -1);
	EXPECT_EQ(lz.prev.at(size - 1), -1);
}

#ifdef USE_GTEST_MAIN
int main(int argc, char** argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
#endif
