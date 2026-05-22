#include <gtest/gtest.h>
#include "../src/qoi.cpp"

TEST(MathTest, AddWorks) {
	EXPECT_EQ(decode(0, nullptr), -1);
}

int main(int argc, char** argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
