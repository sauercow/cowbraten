CXX ?= clang++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra

.PHONY: all run clean
all: demo_library_tests settings_ui_tests profile_ui_tests app_shell_tests chat_ui_tests console_ui_tests console_text_tests

demo_library_tests: demo_library_tests.cpp ../src/cowbraten/ui/demo_library.cpp ../src/cowbraten/ui/demo_library.h
	$(CXX) $(CXXFLAGS) -I../src/cowbraten/ui -o $@ demo_library_tests.cpp ../src/cowbraten/ui/demo_library.cpp -lz

settings_ui_tests: settings_ui_tests.cpp ../data/cowbraten/ui/default/screens/settings.rml ../src/cowbraten/ui/rml_settings.cpp
	$(CXX) $(CXXFLAGS) -o $@ settings_ui_tests.cpp

profile_ui_tests: profile_ui_tests.cpp ../data/cowbraten/ui/default/screens/profile.rml ../data/cowbraten/ui/themes/modern-metallic/theme.rcss ../src/cowbraten/ui/rml_profile.cpp ../src/cowbraten/ui/sauer_profile_bridge.cpp
	$(CXX) $(CXXFLAGS) -o $@ profile_ui_tests.cpp

app_shell_tests: app_shell_tests.cpp ../data/cowbraten/ui/default/templates/app_shell.rml ../data/cowbraten/ui/default/styles/app_shell.rcss
	$(CXX) $(CXXFLAGS) -o $@ app_shell_tests.cpp

chat_ui_tests: chat_ui_tests.cpp ../data/cowbraten/ui/default/screens/chat.rml ../data/cowbraten/ui/default/styles/chat.rcss ../src/cowbraten/ui/rml_chat.cpp ../src/cowbraten/ui/sauer_chat_bridge.cpp
	$(CXX) $(CXXFLAGS) -o $@ chat_ui_tests.cpp

console_ui_tests: console_ui_tests.cpp ../data/cowbraten/ui/default/screens/console.rml ../data/cowbraten/ui/default/styles/console.rcss ../src/cowbraten/ui/rml_console.cpp ../src/cowbraten/ui/sauer_console_bridge.cpp
	$(CXX) $(CXXFLAGS) -o $@ console_ui_tests.cpp

console_text_tests: console_text_tests.cpp ../src/cowbraten/ui/rml_console_text.h
	$(CXX) $(CXXFLAGS) -I../src/cowbraten/ui -o $@ console_text_tests.cpp

run: demo_library_tests settings_ui_tests profile_ui_tests app_shell_tests chat_ui_tests console_ui_tests console_text_tests
	./demo_library_tests
	./settings_ui_tests
	./profile_ui_tests
	./app_shell_tests
	./chat_ui_tests
	./console_ui_tests
	./console_text_tests

clean:
	$(RM) demo_library_tests settings_ui_tests profile_ui_tests app_shell_tests chat_ui_tests console_ui_tests console_text_tests
