#include <gtest/gtest.h>
#include "../src/qoi.cpp"

TEST(QOI_encode, ErrorOnInvalidArguments) {
	EXPECT_EQ(encode(0, nullptr), -1);
}

TEST(QOI_encode, ErrorOnInvalidFilePath) {
	const int ARGC = 2;
	const char* ARGV[] = { "algolab", "data/output.txt" };

	EXPECT_EQ(encode(ARGC, ARGV), -1);
}

TEST(QOI_encode, SucceedOnValidFilePath) {
	const int ARGC = 2;
	const char* ARGV[] = { "algolab", "data/input.txt" };

	EXPECT_EQ(encode(ARGC, ARGV), 0);
}

int main(int argc, char** argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
