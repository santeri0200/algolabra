CPPFLAGS = -std=c++17
WARNINGS = -Wall -Wextra -Wshadow -pedantic
OPTIMISE = -Ofast -march=native -DNDEBUG

main:
	g++ $(CPPFLAGS) $(OPTIMISE) -o out/main src/main.cpp

debug:
	g++ $(CPPFLAGS) $(WARNINGS) -g -o out/main src/main.cpp

clean:
	rm -f out/main
