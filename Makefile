CPPFLAGS = -std=c++17
WARNINGS = -Wall -Wextra -Wshadow -pedantic
OPTIMISE = -Ofast -march=native -DNDEBUG

TESTINGFLAGS  = -Ivendor/googletest/googletest/include -pthread
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

coverage: clean
	$(MAKE) test
	lcov --directory . --capture --output-file coverage.info --ignore-errors inconsistent mismatch
	genhtml coverage.info --output-directory coverage_html

clean:
	rm -f out/main $(TEST_BIN)
	rm -f *.gcno *.gcda *.gcov
	rm -f src/*.gcno src/*.gcda src/*.gcov
	rm -f tests/*.gcno tests/*.gcda tests/*.gcov
	rm -f coverage.info
	rm -rf coverage_html
