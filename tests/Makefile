CXX ?= clang++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra

.PHONY: all run clean rmlui
all: demo_library_tests settings_ui_tests profile_ui_tests app_shell_tests game_menu_ui_tests game_setup_ui_tests system_ui_tests chat_ui_tests console_ui_tests console_text_tests ui_update_tests

RMLUI_BUILD ?= ../build/rmlui-x86_64-linux-Release

rmlui:
	$(MAKE) -C ../src rmlui

ui_update_tests: ui_update_tests.cpp $(RMLUI_BUILD)/librmlui.a
	$(CXX) $(CXXFLAGS) -DRMLUI_STATIC_LIB -I../src/third_party/RmlUi/Include -o $@ $< $(RMLUI_BUILD)/librmlui.a $$(pkg-config --libs freetype2)

$(RMLUI_BUILD)/librmlui.a: rmlui

demo_library_tests: demo_library_tests.cpp ../src/cowbraten/ui/demo_library.cpp ../src/cowbraten/ui/demo_library.h
	$(CXX) $(CXXFLAGS) -I../src/cowbraten/ui -o $@ demo_library_tests.cpp ../src/cowbraten/ui/demo_library.cpp -lz

settings_ui_tests: settings_ui_tests.cpp ../data/cowbraten/ui/default/screens/settings.rml ../src/cowbraten/ui/rml_settings.cpp
	$(CXX) $(CXXFLAGS) -o $@ settings_ui_tests.cpp

profile_ui_tests: profile_ui_tests.cpp ../data/cowbraten/ui/default/screens/profile.rml ../data/cowbraten/ui/themes/modern-metallic/theme.rcss ../src/cowbraten/ui/rml_profile.cpp ../src/cowbraten/ui/sauer_profile_bridge.cpp ../src/fpsgame/client.cpp ../src/cowbraten/detection.cpp
	$(CXX) $(CXXFLAGS) -o $@ profile_ui_tests.cpp

app_shell_tests: app_shell_tests.cpp ../data/cowbraten/ui/default/templates/app_shell.rml ../data/cowbraten/ui/default/styles/app_shell.rcss ../data/cowbraten/ui/default/icons/icons.rcss ../data/cowbraten/ui/default/icons/atlas/cowbraten-icons-24.png ../data/cowbraten/ui/default/icons/atlas/cowbraten-icons-48.png
	$(CXX) $(CXXFLAGS) -o $@ app_shell_tests.cpp

game_menu_ui_tests: game_menu_ui_tests.cpp ../data/cowbraten/ui/default/screens/game_menu.rml ../data/cowbraten/ui/default/styles/game_menu.rcss ../src/cowbraten/ui/rml_game_menu.cpp
	$(CXX) $(CXXFLAGS) -o $@ game_menu_ui_tests.cpp

game_setup_ui_tests: game_setup_ui_tests.cpp ../data/cowbraten/ui/default/screens/games.rml ../data/cowbraten/ui/default/styles/games.rcss ../src/cowbraten/ui/rml_game_setup.cpp ../src/cowbraten/ui/sauer_game_setup_bridge.cpp
	$(CXX) $(CXXFLAGS) -o $@ game_setup_ui_tests.cpp

system_ui_tests: system_ui_tests.cpp ../data/cowbraten/ui/default/screens/system.rml ../data/cowbraten/ui/default/styles/system.rcss ../src/cowbraten/ui/rml_system.cpp ../src/cowbraten/ui/sauer_admin_bridge.cpp
	$(CXX) $(CXXFLAGS) -o $@ system_ui_tests.cpp

chat_ui_tests: chat_ui_tests.cpp ../data/cowbraten/ui/default/screens/chat.rml ../data/cowbraten/ui/default/styles/chat.rcss ../src/cowbraten/ui/rml_chat.cpp ../src/cowbraten/ui/sauer_chat_bridge.cpp
	$(CXX) $(CXXFLAGS) -o $@ chat_ui_tests.cpp

console_ui_tests: console_ui_tests.cpp ../data/cowbraten/ui/default/screens/console.rml ../data/cowbraten/ui/default/styles/console.rcss ../src/cowbraten/ui/rml_console.cpp ../src/cowbraten/ui/sauer_console_bridge.cpp
	$(CXX) $(CXXFLAGS) -o $@ console_ui_tests.cpp

console_text_tests: console_text_tests.cpp ../src/cowbraten/ui/rml_console_text.h
	$(CXX) $(CXXFLAGS) -I../src/cowbraten/ui -o $@ console_text_tests.cpp

run: all
	./demo_library_tests
	./settings_ui_tests
	./profile_ui_tests
	./app_shell_tests
	./game_menu_ui_tests
	./game_setup_ui_tests
	./system_ui_tests
	./chat_ui_tests
	./console_ui_tests
	./console_text_tests
	./ui_update_tests

clean:
	$(RM) demo_library_tests settings_ui_tests profile_ui_tests app_shell_tests game_menu_ui_tests game_setup_ui_tests system_ui_tests chat_ui_tests console_ui_tests console_text_tests ui_update_tests
