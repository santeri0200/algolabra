CPPFLAGS = -std=c++17
WARNINGS = -Wall -Wextra -Wshadow -pedantic
OPTIMISE = -Ofast -march=native -DNDEBUG

TESTINGFLAGS  = -Ithird_party/googletest/googletest/include -pthread
GTEST_LIB_DIR = vendor/googletest/build/lib
GTEST_LIBS    = $(GTEST_LIB_DIR)/libgtest.a $(GTEST_LIB_DIR)/libgtest_main.a
TESTS         = tests/qoi.cpp
TEST_BIN      = out/test

main:
	g++ $(CPPFLAGS) $(OPTIMISE) -o out/main src/main.cpp

debug:
	g++ $(CPPFLAGS) $(WARNINGS) -g -o out/main src/main.cpp

$(TEST_BIN): $(TESTS)
	g++ $(CPPFLAGS) $(WARNINGS) $(TESTINGFLAGS) $^ $(GTEST_LIBS) -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f out/main $(TEST_BIN)
