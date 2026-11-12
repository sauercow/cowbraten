CXX ?= clang++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra

.PHONY: all run clean
all: demo_library_tests

demo_library_tests: demo_library_tests.cpp ../src/cowbraten/ui/demo_library.cpp ../src/cowbraten/ui/demo_library.h
	$(CXX) $(CXXFLAGS) -I../src/cowbraten/ui -o $@ demo_library_tests.cpp ../src/cowbraten/ui/demo_library.cpp -lz

run: demo_library_tests
	./demo_library_tests

clean:
	$(RM) demo_library_tests
