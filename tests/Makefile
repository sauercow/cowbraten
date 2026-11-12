CXX ?= clang++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra

.PHONY: all run clean
all: demo_library_tests settings_ui_tests profile_ui_tests app_shell_tests

demo_library_tests: demo_library_tests.cpp ../src/cowbraten/ui/demo_library.cpp ../src/cowbraten/ui/demo_library.h
	$(CXX) $(CXXFLAGS) -I../src/cowbraten/ui -o $@ demo_library_tests.cpp ../src/cowbraten/ui/demo_library.cpp -lz

settings_ui_tests: settings_ui_tests.cpp ../data/cowbraten/ui/default/screens/settings.rml ../src/cowbraten/ui/rml_settings.cpp
	$(CXX) $(CXXFLAGS) -o $@ settings_ui_tests.cpp

profile_ui_tests: profile_ui_tests.cpp ../data/cowbraten/ui/default/screens/profile.rml ../data/cowbraten/ui/themes/modern-metallic/theme.rcss ../src/cowbraten/ui/rml_profile.cpp ../src/cowbraten/ui/sauer_profile_bridge.cpp
	$(CXX) $(CXXFLAGS) -o $@ profile_ui_tests.cpp

app_shell_tests: app_shell_tests.cpp ../data/cowbraten/ui/default/templates/app_shell.rml ../data/cowbraten/ui/default/styles/app_shell.rcss
	$(CXX) $(CXXFLAGS) -o $@ app_shell_tests.cpp

run: demo_library_tests settings_ui_tests profile_ui_tests app_shell_tests
	./demo_library_tests
	./settings_ui_tests
	./profile_ui_tests
	./app_shell_tests

clean:
	$(RM) demo_library_tests settings_ui_tests profile_ui_tests app_shell_tests
