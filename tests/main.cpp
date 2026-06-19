#include <gtest/gtest.h>
#include "../src/main.cpp"

TEST(QOI_decode, ErrorOnInvalidArguments) {
	EXPECT_EQ(entry(0, nullptr), -1);
}

TEST(QOI_decode, ErrorOnInvalidFilePath) {
	const int ARGC = 3;
	const char* ARGV[] = { "algolab", "qoi", "decode", "data/output.txt" };

	EXPECT_EQ(entry(ARGC, ARGV), -1);
}

TEST(QOI_decode, SucceedOnValidFilePath) {
	const int ARGC = 3;
	const char* ARGV[] = { "algolab", "qoi", "decode", "data/input.txt" };

	EXPECT_EQ(entry(ARGC, ARGV), 0);
}

TEST(PNG_decode, ErrorOnInvalidArguments) {
	EXPECT_EQ(entry(0, nullptr), -1);
}

TEST(PNG_decode, ErrorOnInvalidFilePath) {
	const int ARGC = 3;
	const char* ARGV[] = { "algolab", "png", "decode", "data/output.txt" };

	EXPECT_EQ(entry(ARGC, ARGV), -1);
}

TEST(PNG_decode, SucceedOnValidFilePath) {
	const int ARGC = 3;
	const char* ARGV[] = { "algolab", "png", "decode", "data/input.txt" };

	EXPECT_EQ(entry(ARGC, ARGV), 0);
}

int main(int argc, char** argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
