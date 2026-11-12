CXX ?= clang++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra

.PHONY: all run clean
all: demo_library_tests settings_ui_tests

demo_library_tests: demo_library_tests.cpp ../src/cowbraten/ui/demo_library.cpp ../src/cowbraten/ui/demo_library.h
	$(CXX) $(CXXFLAGS) -I../src/cowbraten/ui -o $@ demo_library_tests.cpp ../src/cowbraten/ui/demo_library.cpp -lz

settings_ui_tests: settings_ui_tests.cpp ../data/cowbraten/ui/default/screens/settings.rml ../src/cowbraten/ui/rml_settings.cpp
	$(CXX) $(CXXFLAGS) -o $@ settings_ui_tests.cpp

run: demo_library_tests settings_ui_tests
	./demo_library_tests
	./settings_ui_tests

clean:
	$(RM) demo_library_tests settings_ui_tests
