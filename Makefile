CPPFLAGS = -std=c++17
WARNINGS = -Wall -Wextra -Wshadow -pedantic
OPTIMISE = -Ofast -march=native -DNDEBUG

TESTINGFLAGS  = -Ithird_party/googletest/googletest/include -pthread
COVERAGEFLAGS = -O0 -g --coverage
GTEST_LIB_DIR = vendor/googletest/build/lib
GTEST_LIBS    = -L$(GTEST_LIB_DIR) -lgtest -lgtest_main
TESTS         = tests/qoi.cpp
TEST_BIN      = out/test

main:
	g++ $(CPPFLAGS) $(OPTIMISE) -o out/main src/main.cpp

debug:
	g++ $(CPPFLAGS) $(WARNINGS) -g -o out/main src/main.cpp

$(TEST_BIN): $(TESTS)
	mkdir -p out
	g++ $(CPPFLAGS) $(WARNINGS) $(TESTINGFLAGS) $(COVERAGEFLAGS) $^ $(GTEST_LIBS) -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

coverage: test
	lcov --directory . --capture --output-file coverage.info
	lcov --remove coverage.info '/usr/*' '*/third_party/*' '*/vendor/*' '*/tests/*' --output-file coverage.info
	genhtml coverage.info --output-directory coverage_html

clean:
	rm -f out/main $(TEST_BIN)
