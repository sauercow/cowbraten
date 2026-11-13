# building ON Windows is not supported, only building FOR Windows (set TARGET before calling make)
HOSTOS=linux
ifneq (,$(findstring Darwin,$(shell uname -s)))
	HOSTOS=macos
endif

ifndef TARGET
	# Only x86-64 targets are currently supported.
	TARGET=x86_64-$(HOSTOS)
endif

ifeq (,$(findstring $(HOSTOS),$(TARGET)))
	CC=zig cc --target=$(TARGET)
	CXX=zig c++ --target=$(TARGET)
	AR=zig ar
else
	CC=clang
	CXX=clang++
endif

CFLAGS=
CXXFLAGS= -std=c++17 -fomit-frame-pointer -ffast-math -Wno-deprecated-declarations
LINKERFLAGS=
ifdef DEBUG
	override CFLAGS+= -g3
	override CXXFLAGS+= -O0 -g3 -DDEBUG=$$DEBUG
	RMLUI_BUILD_TYPE=Debug
else
	override CXXFLAGS+= -O3
	override LINKERFLAGS+= -Wl,-s
	RMLUI_BUILD_TYPE=Release
endif

override CXXFLAGS+= -Wall -fsigned-char -fno-exceptions -fno-rtti
ifneq (,$(findstring macos,$(TARGET)))
	OSXMIN=10.12 # minimum supported macOS version
	override CFLAGS+= -mmacosx-version-min=$(OSXMIN)
	override CXXFLAGS+= -mmacosx-version-min=$(OSXMIN)
	override LINKERFLAGS+= -Wl,-rpath,@executable_path/../Frameworks
endif

INCLUDES= -Ishared -Iengine -Ifpsgame -Icowbraten -Ienet/include
LIBS= -Lenet -lenet

RMLUI_DIR=third_party/RmlUi
RMLUI_BUILD=../build/rmlui-$(TARGET)-$(RMLUI_BUILD_TYPE)
RMLUI_LIB=$(RMLUI_BUILD)/librmlui.a
RMLUI_LINK=

ifneq (,$(findstring windows,$(TARGET)))
	INCLUDES+= -Iwindows/include/zlib
	LIBS+= -L../bin64 -Lwindows/lib64 -lzlib1 -lws2_32 -lwinmm
	CLIENT_INCLUDES= -Iwindows/include/SDL2
	CLIENT_LIBS= -lSDL2 -lSDL2_image -lSDL2_mixer -lopengl32
	CLIENT_RESOURCES= windows/cowbraten.manifest
endif

ifneq (,$(findstring macos,$(TARGET)))
	INCLUDES+= -Imacos/sdk_11.3/usr/include
	LIBS+= -Lmacos/sdk_11.3/usr/lib -lz
	CLIENT_INCLUDES= -Imacos/include
	CLIENT_LIBS= -F./macos/sdk_11.3/System/Library/Frameworks -framework OpenGL
	CLIENT_LIBS+= -F../sauerbraten.app/Contents/Frameworks -framework SDL2 -framework SDL2_image -framework SDL2_mixer
endif

ifneq (,$(findstring linux,$(TARGET)))
	INCLUDES+= -I/usr/include
	LIBS+= -L/usr/lib64 -lz
	CLIENT_INCLUDES= `sdl2-config --cflags`
	CLIENT_LIBS= -lX11 `sdl2-config --libs` -lSDL2_image -lSDL2_mixer -lGL -lrt

	RMLUI_ENABLED=1
	RMLUI_LINK=$(RMLUI_LIB) $(shell pkg-config --libs freetype2)
endif

ifndef CLIENT_INCLUDES
$(error unsupported target $(TARGET))
endif

UI_OBJS= \
	cowbraten/ui/ui.o \
	cowbraten/ui/rml_action.o \
	cowbraten/ui/rml_chat.o \
	cowbraten/ui/rml_console.o \
	cowbraten/ui/rml_render.o \
	cowbraten/ui/rml_file.o \
	cowbraten/ui/demo_library.o \
	cowbraten/ui/rml_demo_manager.o \
	cowbraten/ui/rml_demo_player.o \
	cowbraten/ui/rml_game_menu.o \
	cowbraten/ui/rml_game_setup.o \
	cowbraten/ui/rml_profile.o \
	cowbraten/ui/rml_server_browser.o \
	cowbraten/ui/rml_settings.o \
	cowbraten/ui/profile_api.o \
	cowbraten/ui/game_menu_api.o \
	cowbraten/ui/settings_api.o \
	cowbraten/ui/keybindings_api.o \
	cowbraten/ui/sauer_chat_bridge.o \
	cowbraten/ui/sauer_console_bridge.o \
	cowbraten/ui/sauer_file_bridge.o \
	cowbraten/ui/sauer_demo_bridge.o \
	cowbraten/ui/sauer_input_bridge.o \
	cowbraten/ui/sauer_game_menu_bridge.o \
	cowbraten/ui/game_setup_api.o \
	cowbraten/ui/sauer_game_setup_bridge.o \
	cowbraten/ui/sauer_profile_bridge.o \
	cowbraten/ui/sauer_server_bridge.o \
	cowbraten/ui/sauer_settings_bridge.o \
	cowbraten/ui/sauer_keybindings_bridge.o \
	cowbraten/ui/ui_commands.o

CLIENT_OBJS= \
	$(UI_OBJS) \
	shared/crypto.o \
	shared/geom.o \
	shared/glemu.o \
	shared/stream.o \
	shared/tools.o \
	shared/zip.o \
	engine/3dgui.o \
	engine/bih.o \
	engine/blend.o \
	engine/blob.o \
	engine/client.o	\
	engine/command.o \
	engine/console.o \
	engine/cubeloader.o \
	engine/decal.o \
	engine/dynlight.o \
	engine/glare.o \
	engine/grass.o \
	engine/lightmap.o \
	engine/main.o \
	engine/material.o \
	engine/menus.o \
	engine/movie.o \
	engine/normal.o	\
	engine/octa.o \
	engine/octaedit.o \
	engine/octarender.o \
	engine/physics.o \
	engine/pvs.o \
	engine/rendergl.o \
	engine/rendermodel.o \
	engine/renderparticles.o \
	engine/rendersky.o \
	engine/rendertext.o \
	engine/renderva.o \
	engine/server.o	\
	engine/serverbrowser.o \
	engine/shader.o \
	engine/shadowmap.o \
	engine/sound.o \
	engine/texture.o \
	engine/water.o \
	engine/world.o \
	engine/worldio.o \
	fpsgame/ai.o \
	fpsgame/client.o \
	fpsgame/entities.o \
	fpsgame/fps.o \
	fpsgame/monster.o \
	fpsgame/movable.o \
	fpsgame/render.o \
	fpsgame/scoreboard.o \
	fpsgame/server.o \
	fpsgame/waypoint.o \
	fpsgame/weapon.o \
	cowbraten/bans.o \
	cowbraten/capability_probing.o \
	cowbraten/clientdemo.o \
	cowbraten/demohd.o \
	cowbraten/demo_info_message.o \
	cowbraten/detection.o \
	cowbraten/embedded_cfgs.o \
	cowbraten/fragmessages.o \
	cowbraten/fullscreenmonitor.o \
	cowbraten/managed_games.o \
	cowbraten/namecomplete.o \
	cowbraten/proxy_real_ip.o \
	cowbraten/remote_commands.o \
	cowbraten/scoreboard.o \
	cowbraten/serverbrowser.o \
	cowbraten/version.o \
	cowbraten/weaponstats.o
SERVER_OBJS= \
	shared/crypto-standalone.o \
	shared/stream-standalone.o \
	shared/tools-standalone.o \
	engine/command-standalone.o \
	engine/server-standalone.o \
	engine/worldio-standalone.o \
	fpsgame/entities-standalone.o \
	fpsgame/server-standalone.o \
	cowbraten/bans-standalone.o \
	cowbraten/demo_info_message-standalone.o \
	cowbraten/managed_games-standalone.o \
	cowbraten/proxy_real_ip-standalone.o \
	cowbraten/remote_commands-standalone.o \
	cowbraten/version-standalone.o
DEP_FILES=$(CLIENT_OBJS:.o=.d) $(SERVER_OBJS:.o=.d)

default: all

all: client server

clean:
	cd enet && make clean && cd ..
	-$(RM) $(CLIENT_OBJS) $(SERVER_OBJS) $(DEP_FILES) cowbraten_client cowbraten_server

%-standalone.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(CLIENT_OBJS): CXXFLAGS += $(INCLUDES) $(CLIENT_INCLUDES) -MMD -MP

cowbraten/ui/ui.o cowbraten/ui/rml_action.o cowbraten/ui/rml_chat.o cowbraten/ui/rml_console.o cowbraten/ui/rml_render.o cowbraten/ui/rml_file.o cowbraten/ui/rml_server_browser.o cowbraten/ui/rml_demo_manager.o cowbraten/ui/rml_demo_player.o cowbraten/ui/rml_game_menu.o cowbraten/ui/rml_game_setup.o cowbraten/ui/rml_profile.o cowbraten/ui/rml_settings.o: CXXFLAGS += -Ithird_party/RmlUi/Include -DRMLUI_STATIC_LIB -fexceptions -frtti

$(SERVER_OBJS): CXXFLAGS += $(INCLUDES) -DSTANDALONE -MMD -MP

-include $(DEP_FILES)

OUTBIN=cowbraten_x86_64

ifdef RMLUI_ENABLED
rmlui:
	@if [ ! -f "$(RMLUI_BUILD)/CMakeCache.txt" ]; then \
		cmake -S $(RMLUI_DIR) -B $(RMLUI_BUILD) \
			-DCMAKE_C_COMPILER=clang \
			-DCMAKE_CXX_COMPILER=clang++ \
			-DBUILD_SHARED_LIBS=OFF \
			-DBUILD_TESTING=OFF \
			-DRMLUI_SAMPLES=OFF \
			-DRMLUI_LUA_BINDINGS=OFF \
			-DRMLUI_FONT_ENGINE=freetype \
			-DCMAKE_BUILD_TYPE=$(RMLUI_BUILD_TYPE); \
	fi
	cmake --build $(RMLUI_BUILD) --target rmlui_core --parallel

client: rmlui
.PHONY: rmlui
endif

client:	libenet
client: $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) $(LINKERFLAGS) -o cowbraten_client $(CLIENT_OBJS) $(CLIENT_RESOURCES) $(LIBS) $(CLIENT_LIBS) $(RMLUI_LINK)

server:	libenet
server: $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) $(LINKERFLAGS) -o cowbraten_server $(SERVER_OBJS) $(LIBS)

install: all
ifneq (,$(findstring windows,$(TARGET)))
	cp cowbraten_client ../bin64/$(OUTBIN)_client.exe
	cp cowbraten_server ../bin64/$(OUTBIN)_server.exe
else
ifneq (,$(findstring macos,$(TARGET)))
	cp cowbraten_client ../sauerbraten.app/Contents/MacOS/$(OUTBIN)_client
	cp cowbraten_server ../sauerbraten.app/Contents/MacOS/$(OUTBIN)_server
else # linux
	cp cowbraten_client ../bin_unix/$(OUTBIN)_client
	cp cowbraten_server ../bin_unix/$(OUTBIN)_server
endif
endif

enet/libenet.a:
	$(MAKE) -C enet CC='$(CC)' AR='$(AR)'
libenet: enet/libenet.a

.PHONY: libenet

depend:
	makedepend -Y -Ishared -Iengine -Ifpsgame -I cowbraten $(CLIENT_OBJS:.o=.cpp)
	makedepend -a -o-standalone.o -Y -DSTANDALONE -Ishared -Iengine -Ifpsgame -Icowbraten $(SERVER_OBJS:-standalone.o=.cpp)

# DO NOT DELETE

shared/crypto.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/crypto.o: shared/command.h cowbraten/colors.h shared/glexts.h
shared/crypto.o: shared/glemu.h shared/iengine.h shared/igame.h
shared/crypto.o: cowbraten/authservers.h cowbraten/masterserver.h
shared/geom.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/geom.o: shared/command.h cowbraten/colors.h shared/glexts.h
shared/geom.o: shared/glemu.h shared/iengine.h shared/igame.h
shared/geom.o: cowbraten/authservers.h cowbraten/masterserver.h
shared/glemu.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/glemu.o: shared/command.h cowbraten/colors.h shared/glexts.h
shared/glemu.o: shared/glemu.h shared/iengine.h shared/igame.h
shared/glemu.o: cowbraten/authservers.h cowbraten/masterserver.h
shared/stream.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/stream.o: shared/command.h cowbraten/colors.h shared/glexts.h
shared/stream.o: shared/glemu.h shared/iengine.h shared/igame.h
shared/stream.o: cowbraten/authservers.h cowbraten/masterserver.h
shared/tools.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/tools.o: shared/command.h cowbraten/colors.h shared/glexts.h
shared/tools.o: shared/glemu.h shared/iengine.h shared/igame.h
shared/tools.o: cowbraten/authservers.h cowbraten/masterserver.h
shared/zip.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/zip.o: shared/command.h cowbraten/colors.h shared/glexts.h
shared/zip.o: shared/glemu.h shared/iengine.h shared/igame.h
shared/zip.o: cowbraten/authservers.h cowbraten/masterserver.h
engine/3dgui.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/3dgui.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/3dgui.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/3dgui.o: shared/igame.h cowbraten/authservers.h
engine/3dgui.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/3dgui.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/3dgui.o: engine/model.h engine/textedit.h
engine/bih.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/bih.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/bih.o: shared/glexts.h shared/glemu.h shared/iengine.h shared/igame.h
engine/bih.o: cowbraten/authservers.h cowbraten/masterserver.h engine/world.h
engine/bih.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/bih.o: engine/model.h
engine/blend.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/blend.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/blend.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/blend.o: shared/igame.h cowbraten/authservers.h
engine/blend.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/blend.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/blend.o: engine/model.h
engine/blob.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/blob.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/blob.o: shared/glexts.h shared/glemu.h shared/iengine.h shared/igame.h
engine/blob.o: cowbraten/authservers.h cowbraten/masterserver.h
engine/blob.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/blob.o: engine/texture.h engine/model.h
engine/client.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/client.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/client.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/client.o: shared/igame.h cowbraten/authservers.h
engine/client.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/client.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/client.o: engine/model.h
engine/command.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/command.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/command.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/command.o: shared/igame.h cowbraten/authservers.h
engine/command.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/command.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/command.o: engine/model.h
engine/console.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/console.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/console.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/console.o: shared/igame.h cowbraten/authservers.h
engine/console.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/console.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/console.o: engine/model.h
engine/cubeloader.o: engine/engine.h shared/cube.h shared/tools.h
engine/cubeloader.o: shared/geom.h shared/ents.h shared/command.h
engine/cubeloader.o: cowbraten/colors.h shared/glexts.h shared/glemu.h
engine/cubeloader.o: shared/iengine.h shared/igame.h cowbraten/authservers.h
engine/cubeloader.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/cubeloader.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/cubeloader.o: engine/model.h
engine/decal.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/decal.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/decal.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/decal.o: shared/igame.h cowbraten/authservers.h
engine/decal.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/decal.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/decal.o: engine/model.h
engine/dynlight.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/dynlight.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/dynlight.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/dynlight.o: shared/igame.h cowbraten/authservers.h
engine/dynlight.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/dynlight.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/dynlight.o: engine/model.h
engine/glare.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/glare.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/glare.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/glare.o: shared/igame.h cowbraten/authservers.h
engine/glare.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/glare.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/glare.o: engine/model.h engine/rendertarget.h
engine/grass.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/grass.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/grass.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/grass.o: shared/igame.h cowbraten/authservers.h
engine/grass.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/grass.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/grass.o: engine/model.h
engine/lightmap.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/lightmap.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/lightmap.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/lightmap.o: shared/igame.h cowbraten/authservers.h
engine/lightmap.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/lightmap.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/lightmap.o: engine/model.h
engine/main.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/main.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/main.o: shared/glexts.h shared/glemu.h shared/iengine.h shared/igame.h
engine/main.o: cowbraten/authservers.h cowbraten/masterserver.h
engine/main.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/main.o: engine/texture.h engine/model.h
engine/material.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/material.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/material.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/material.o: shared/igame.h cowbraten/authservers.h
engine/material.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/material.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/material.o: engine/model.h
engine/menus.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/menus.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/menus.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/menus.o: shared/igame.h cowbraten/authservers.h
engine/menus.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/menus.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/menus.o: engine/model.h
engine/movie.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/movie.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/movie.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/movie.o: shared/igame.h cowbraten/authservers.h
engine/movie.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/movie.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/movie.o: engine/model.h
engine/normal.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/normal.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/normal.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/normal.o: shared/igame.h cowbraten/authservers.h
engine/normal.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/normal.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/normal.o: engine/model.h
engine/octa.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/octa.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/octa.o: shared/glexts.h shared/glemu.h shared/iengine.h shared/igame.h
engine/octa.o: cowbraten/authservers.h cowbraten/masterserver.h
engine/octa.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/octa.o: engine/texture.h engine/model.h
engine/octaedit.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/octaedit.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/octaedit.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/octaedit.o: shared/igame.h cowbraten/authservers.h
engine/octaedit.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/octaedit.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/octaedit.o: engine/model.h
engine/octarender.o: engine/engine.h shared/cube.h shared/tools.h
engine/octarender.o: shared/geom.h shared/ents.h shared/command.h
engine/octarender.o: cowbraten/colors.h shared/glexts.h shared/glemu.h
engine/octarender.o: shared/iengine.h shared/igame.h cowbraten/authservers.h
engine/octarender.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/octarender.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/octarender.o: engine/model.h
engine/physics.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/physics.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/physics.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/physics.o: shared/igame.h cowbraten/authservers.h
engine/physics.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/physics.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/physics.o: engine/model.h engine/mpr.h
engine/pvs.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/pvs.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/pvs.o: shared/glexts.h shared/glemu.h shared/iengine.h shared/igame.h
engine/pvs.o: cowbraten/authservers.h cowbraten/masterserver.h engine/world.h
engine/pvs.o: engine/octa.h engine/lightmap.h engine/bih.h engine/texture.h
engine/pvs.o: engine/model.h
engine/rendergl.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/rendergl.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/rendergl.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/rendergl.o: shared/igame.h cowbraten/authservers.h
engine/rendergl.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/rendergl.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/rendergl.o: engine/model.h
engine/rendermodel.o: engine/engine.h shared/cube.h shared/tools.h
engine/rendermodel.o: shared/geom.h shared/ents.h shared/command.h
engine/rendermodel.o: cowbraten/colors.h shared/glexts.h shared/glemu.h
engine/rendermodel.o: shared/iengine.h shared/igame.h cowbraten/authservers.h
engine/rendermodel.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/rendermodel.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/rendermodel.o: engine/model.h engine/ragdoll.h engine/animmodel.h
engine/rendermodel.o: engine/vertmodel.h engine/skelmodel.h engine/md2.h
engine/rendermodel.o: engine/md3.h engine/md5.h engine/obj.h engine/smd.h
engine/rendermodel.o: engine/iqm.h
engine/renderparticles.o: engine/engine.h shared/cube.h shared/tools.h
engine/renderparticles.o: shared/geom.h shared/ents.h shared/command.h
engine/renderparticles.o: cowbraten/colors.h shared/glexts.h shared/glemu.h
engine/renderparticles.o: shared/iengine.h shared/igame.h
engine/renderparticles.o: cowbraten/authservers.h cowbraten/masterserver.h
engine/renderparticles.o: engine/world.h engine/octa.h engine/lightmap.h
engine/renderparticles.o: engine/bih.h engine/texture.h engine/model.h
engine/renderparticles.o: engine/rendertarget.h engine/depthfx.h
engine/renderparticles.o: engine/explosion.h engine/lensflare.h
engine/renderparticles.o: engine/lightning.h
engine/rendersky.o: engine/engine.h shared/cube.h shared/tools.h
engine/rendersky.o: shared/geom.h shared/ents.h shared/command.h
engine/rendersky.o: cowbraten/colors.h shared/glexts.h shared/glemu.h
engine/rendersky.o: shared/iengine.h shared/igame.h cowbraten/authservers.h
engine/rendersky.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/rendersky.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/rendersky.o: engine/model.h
engine/rendertext.o: engine/engine.h shared/cube.h shared/tools.h
engine/rendertext.o: shared/geom.h shared/ents.h shared/command.h
engine/rendertext.o: cowbraten/colors.h shared/glexts.h shared/glemu.h
engine/rendertext.o: shared/iengine.h shared/igame.h cowbraten/authservers.h
engine/rendertext.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/rendertext.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/rendertext.o: engine/model.h
engine/renderva.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/renderva.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/renderva.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/renderva.o: shared/igame.h cowbraten/authservers.h
engine/renderva.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/renderva.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/renderva.o: engine/model.h
engine/server.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/server.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/server.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/server.o: shared/igame.h cowbraten/authservers.h
engine/server.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/server.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/server.o: engine/model.h
engine/serverbrowser.o: engine/engine.h shared/cube.h shared/tools.h
engine/serverbrowser.o: shared/geom.h shared/ents.h shared/command.h
engine/serverbrowser.o: cowbraten/colors.h shared/glexts.h shared/glemu.h
engine/serverbrowser.o: shared/iengine.h shared/igame.h
engine/serverbrowser.o: cowbraten/authservers.h cowbraten/masterserver.h
engine/serverbrowser.o: engine/world.h engine/octa.h engine/lightmap.h
engine/serverbrowser.o: engine/bih.h engine/texture.h engine/model.h
engine/serverbrowser.o: cowbraten/serverbrowser.h
engine/shader.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/shader.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/shader.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/shader.o: shared/igame.h cowbraten/authservers.h
engine/shader.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/shader.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/shader.o: engine/model.h
engine/shadowmap.o: engine/engine.h shared/cube.h shared/tools.h
engine/shadowmap.o: shared/geom.h shared/ents.h shared/command.h
engine/shadowmap.o: cowbraten/colors.h shared/glexts.h shared/glemu.h
engine/shadowmap.o: shared/iengine.h shared/igame.h cowbraten/authservers.h
engine/shadowmap.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/shadowmap.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/shadowmap.o: engine/model.h engine/rendertarget.h
engine/sound.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/sound.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/sound.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/sound.o: shared/igame.h cowbraten/authservers.h
engine/sound.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/sound.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/sound.o: engine/model.h
engine/texture.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/texture.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/texture.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/texture.o: shared/igame.h cowbraten/authservers.h
engine/texture.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/texture.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/texture.o: engine/model.h
engine/water.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/water.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/water.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/water.o: shared/igame.h cowbraten/authservers.h
engine/water.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/water.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/water.o: engine/model.h
engine/world.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/world.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/world.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/world.o: shared/igame.h cowbraten/authservers.h
engine/world.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/world.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/world.o: engine/model.h
engine/worldio.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/worldio.o: shared/ents.h shared/command.h cowbraten/colors.h
engine/worldio.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/worldio.o: shared/igame.h cowbraten/authservers.h
engine/worldio.o: cowbraten/masterserver.h engine/world.h engine/octa.h
engine/worldio.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/worldio.o: engine/model.h
fpsgame/ai.o: fpsgame/game.h shared/cube.h shared/tools.h shared/geom.h
fpsgame/ai.o: shared/ents.h shared/command.h cowbraten/colors.h
fpsgame/ai.o: shared/glexts.h shared/glemu.h shared/iengine.h shared/igame.h
fpsgame/ai.o: cowbraten/authservers.h cowbraten/masterserver.h fpsgame/ai.h
fpsgame/ai.o: cowbraten/weaponstats_type.h cowbraten/fragmessages.h
fpsgame/client.o: fpsgame/game.h shared/cube.h shared/tools.h shared/geom.h
fpsgame/client.o: shared/ents.h shared/command.h cowbraten/colors.h
fpsgame/client.o: shared/glexts.h shared/glemu.h shared/iengine.h
fpsgame/client.o: shared/igame.h cowbraten/authservers.h
fpsgame/client.o: cowbraten/masterserver.h fpsgame/ai.h
fpsgame/client.o: cowbraten/weaponstats_type.h cowbraten/fragmessages.h
fpsgame/client.o: cowbraten/weaponstats.h fpsgame/capture.h fpsgame/ctf.h
fpsgame/client.o: fpsgame/collect.h
fpsgame/entities.o: fpsgame/game.h shared/cube.h shared/tools.h shared/geom.h
fpsgame/entities.o: shared/ents.h shared/command.h cowbraten/colors.h
fpsgame/entities.o: shared/glexts.h shared/glemu.h shared/iengine.h
fpsgame/entities.o: shared/igame.h cowbraten/authservers.h
fpsgame/entities.o: cowbraten/masterserver.h fpsgame/ai.h
fpsgame/entities.o: cowbraten/weaponstats_type.h cowbraten/fragmessages.h
fpsgame/fps.o: fpsgame/game.h shared/cube.h shared/tools.h shared/geom.h
fpsgame/fps.o: shared/ents.h shared/command.h cowbraten/colors.h
fpsgame/fps.o: shared/glexts.h shared/glemu.h shared/iengine.h shared/igame.h
fpsgame/fps.o: cowbraten/authservers.h cowbraten/masterserver.h fpsgame/ai.h
fpsgame/fps.o: cowbraten/weaponstats_type.h cowbraten/fragmessages.h
fpsgame/fps.o: cowbraten/weaponstats.h
fpsgame/monster.o: fpsgame/game.h shared/cube.h shared/tools.h shared/geom.h
fpsgame/monster.o: shared/ents.h shared/command.h cowbraten/colors.h
fpsgame/monster.o: shared/glexts.h shared/glemu.h shared/iengine.h
fpsgame/monster.o: shared/igame.h cowbraten/authservers.h
fpsgame/monster.o: cowbraten/masterserver.h fpsgame/ai.h
fpsgame/monster.o: cowbraten/weaponstats_type.h cowbraten/fragmessages.h
fpsgame/movable.o: fpsgame/game.h shared/cube.h shared/tools.h shared/geom.h
fpsgame/movable.o: shared/ents.h shared/command.h cowbraten/colors.h
fpsgame/movable.o: shared/glexts.h shared/glemu.h shared/iengine.h
fpsgame/movable.o: shared/igame.h cowbraten/authservers.h
fpsgame/movable.o: cowbraten/masterserver.h fpsgame/ai.h
fpsgame/movable.o: cowbraten/weaponstats_type.h cowbraten/fragmessages.h
fpsgame/render.o: fpsgame/game.h shared/cube.h shared/tools.h shared/geom.h
fpsgame/render.o: shared/ents.h shared/command.h cowbraten/colors.h
fpsgame/render.o: shared/glexts.h shared/glemu.h shared/iengine.h
fpsgame/render.o: shared/igame.h cowbraten/authservers.h
fpsgame/render.o: cowbraten/masterserver.h fpsgame/ai.h
fpsgame/render.o: cowbraten/weaponstats_type.h cowbraten/fragmessages.h
fpsgame/scoreboard.o: fpsgame/game.h shared/cube.h shared/tools.h
fpsgame/scoreboard.o: shared/geom.h shared/ents.h shared/command.h
fpsgame/scoreboard.o: cowbraten/colors.h shared/glexts.h shared/glemu.h
fpsgame/scoreboard.o: shared/iengine.h shared/igame.h cowbraten/authservers.h
fpsgame/scoreboard.o: cowbraten/masterserver.h fpsgame/ai.h
fpsgame/scoreboard.o: cowbraten/weaponstats_type.h cowbraten/fragmessages.h
fpsgame/server.o: fpsgame/game.h shared/cube.h shared/tools.h shared/geom.h
fpsgame/server.o: shared/ents.h shared/command.h cowbraten/colors.h
fpsgame/server.o: shared/glexts.h shared/glemu.h shared/iengine.h
fpsgame/server.o: shared/igame.h cowbraten/authservers.h
fpsgame/server.o: cowbraten/masterserver.h fpsgame/ai.h
fpsgame/server.o: cowbraten/weaponstats_type.h cowbraten/fragmessages.h
fpsgame/server.o: fpsgame/capture.h fpsgame/ctf.h fpsgame/collect.h
fpsgame/server.o: fpsgame/extinfo.h fpsgame/aiman.h
fpsgame/waypoint.o: fpsgame/game.h shared/cube.h shared/tools.h shared/geom.h
fpsgame/waypoint.o: shared/ents.h shared/command.h cowbraten/colors.h
fpsgame/waypoint.o: shared/glexts.h shared/glemu.h shared/iengine.h
fpsgame/waypoint.o: shared/igame.h cowbraten/authservers.h
fpsgame/waypoint.o: cowbraten/masterserver.h fpsgame/ai.h
fpsgame/waypoint.o: cowbraten/weaponstats_type.h cowbraten/fragmessages.h
fpsgame/weapon.o: fpsgame/game.h shared/cube.h shared/tools.h shared/geom.h
fpsgame/weapon.o: shared/ents.h shared/command.h cowbraten/colors.h
fpsgame/weapon.o: shared/glexts.h shared/glemu.h shared/iengine.h
fpsgame/weapon.o: shared/igame.h cowbraten/authservers.h
fpsgame/weapon.o: cowbraten/masterserver.h fpsgame/ai.h
fpsgame/weapon.o: cowbraten/weaponstats_type.h cowbraten/fragmessages.h
fpsgame/weapon.o: cowbraten/weaponstats.h
cowbraten/bans.o: fpsgame/game.h shared/cube.h shared/tools.h shared/geom.h
cowbraten/bans.o: shared/ents.h shared/command.h cowbraten/colors.h
cowbraten/bans.o: shared/glexts.h shared/glemu.h shared/iengine.h
cowbraten/bans.o: shared/igame.h cowbraten/authservers.h
cowbraten/bans.o: cowbraten/masterserver.h fpsgame/ai.h
cowbraten/bans.o: cowbraten/weaponstats_type.h cowbraten/fragmessages.h
cowbraten/capability_probing.o: fpsgame/game.h shared/cube.h shared/tools.h
cowbraten/capability_probing.o: shared/geom.h shared/ents.h shared/command.h
cowbraten/capability_probing.o: cowbraten/colors.h shared/glexts.h
cowbraten/capability_probing.o: shared/glemu.h shared/iengine.h
cowbraten/capability_probing.o: shared/igame.h cowbraten/authservers.h
cowbraten/capability_probing.o: cowbraten/masterserver.h fpsgame/ai.h
cowbraten/capability_probing.o: cowbraten/weaponstats_type.h
cowbraten/capability_probing.o: cowbraten/fragmessages.h
cowbraten/clientdemo.o: fpsgame/game.h shared/cube.h shared/tools.h
cowbraten/clientdemo.o: shared/geom.h shared/ents.h shared/command.h
cowbraten/clientdemo.o: cowbraten/colors.h shared/glexts.h shared/glemu.h
cowbraten/clientdemo.o: shared/iengine.h shared/igame.h
cowbraten/clientdemo.o: cowbraten/authservers.h cowbraten/masterserver.h
cowbraten/clientdemo.o: fpsgame/ai.h cowbraten/weaponstats_type.h
cowbraten/clientdemo.o: cowbraten/fragmessages.h
cowbraten/demo_info_message.o: fpsgame/game.h shared/cube.h shared/tools.h
cowbraten/demo_info_message.o: shared/geom.h shared/ents.h shared/command.h
cowbraten/demo_info_message.o: cowbraten/colors.h shared/glexts.h
cowbraten/demo_info_message.o: shared/glemu.h shared/iengine.h shared/igame.h
cowbraten/demo_info_message.o: cowbraten/authservers.h
cowbraten/demo_info_message.o: cowbraten/masterserver.h fpsgame/ai.h
cowbraten/demo_info_message.o: cowbraten/weaponstats_type.h
cowbraten/demo_info_message.o: cowbraten/fragmessages.h
cowbraten/detection.o: fpsgame/game.h shared/cube.h shared/tools.h
cowbraten/detection.o: shared/geom.h shared/ents.h shared/command.h
cowbraten/detection.o: cowbraten/colors.h shared/glexts.h shared/glemu.h
cowbraten/detection.o: shared/iengine.h shared/igame.h
cowbraten/detection.o: cowbraten/authservers.h cowbraten/masterserver.h
cowbraten/detection.o: fpsgame/ai.h cowbraten/weaponstats_type.h
cowbraten/detection.o: cowbraten/fragmessages.h
cowbraten/embedded_cfgs.o: shared/cube.h shared/tools.h shared/geom.h
cowbraten/embedded_cfgs.o: shared/ents.h shared/command.h cowbraten/colors.h
cowbraten/embedded_cfgs.o: shared/glexts.h shared/glemu.h shared/iengine.h
cowbraten/embedded_cfgs.o: shared/igame.h cowbraten/authservers.h
cowbraten/embedded_cfgs.o: cowbraten/masterserver.h cowbraten/embedded_cfgs.h
cowbraten/embedded_cfgs.o: ../data/cowbraten/menus.cfg.gz.xxd
cowbraten/embedded_cfgs.o: ../data/cowbraten/master.cfg.gz.xxd
cowbraten/embedded_cfgs.o: ../data/cowbraten/gamehud.cfg.gz.xxd
cowbraten/embedded_cfgs.o: ../data/cowbraten/keymap.cfg.gz.xxd
cowbraten/fragmessages.o: fpsgame/game.h shared/cube.h shared/tools.h
cowbraten/fragmessages.o: shared/geom.h shared/ents.h shared/command.h
cowbraten/fragmessages.o: cowbraten/colors.h shared/glexts.h shared/glemu.h
cowbraten/fragmessages.o: shared/iengine.h shared/igame.h
cowbraten/fragmessages.o: cowbraten/authservers.h cowbraten/masterserver.h
cowbraten/fragmessages.o: fpsgame/ai.h cowbraten/weaponstats_type.h
cowbraten/fragmessages.o: cowbraten/fragmessages.h
cowbraten/fullscreenmonitor.o: engine/engine.h shared/cube.h shared/tools.h
cowbraten/fullscreenmonitor.o: shared/geom.h shared/ents.h shared/command.h
cowbraten/fullscreenmonitor.o: cowbraten/colors.h shared/glexts.h
cowbraten/fullscreenmonitor.o: shared/glemu.h shared/iengine.h shared/igame.h
cowbraten/fullscreenmonitor.o: cowbraten/authservers.h
cowbraten/fullscreenmonitor.o: cowbraten/masterserver.h engine/world.h
cowbraten/fullscreenmonitor.o: engine/octa.h engine/lightmap.h engine/bih.h
cowbraten/fullscreenmonitor.o: engine/texture.h engine/model.h
cowbraten/managed_games.o: fpsgame/game.h shared/cube.h shared/tools.h
cowbraten/managed_games.o: shared/geom.h shared/ents.h shared/command.h
cowbraten/managed_games.o: cowbraten/colors.h shared/glexts.h shared/glemu.h
cowbraten/managed_games.o: shared/iengine.h shared/igame.h
cowbraten/managed_games.o: cowbraten/authservers.h cowbraten/masterserver.h
cowbraten/managed_games.o: fpsgame/ai.h cowbraten/weaponstats_type.h
cowbraten/managed_games.o: cowbraten/fragmessages.h
cowbraten/namecomplete.o: fpsgame/game.h shared/cube.h shared/tools.h
cowbraten/namecomplete.o: shared/geom.h shared/ents.h shared/command.h
cowbraten/namecomplete.o: cowbraten/colors.h shared/glexts.h shared/glemu.h
cowbraten/namecomplete.o: shared/iengine.h shared/igame.h
cowbraten/namecomplete.o: cowbraten/authservers.h cowbraten/masterserver.h
cowbraten/namecomplete.o: fpsgame/ai.h cowbraten/weaponstats_type.h
cowbraten/namecomplete.o: cowbraten/fragmessages.h
cowbraten/proxy_real_ip.o: fpsgame/game.h shared/cube.h shared/tools.h
cowbraten/proxy_real_ip.o: shared/geom.h shared/ents.h shared/command.h
cowbraten/proxy_real_ip.o: cowbraten/colors.h shared/glexts.h shared/glemu.h
cowbraten/proxy_real_ip.o: shared/iengine.h shared/igame.h
cowbraten/proxy_real_ip.o: cowbraten/authservers.h cowbraten/masterserver.h
cowbraten/proxy_real_ip.o: fpsgame/ai.h cowbraten/weaponstats_type.h
cowbraten/proxy_real_ip.o: cowbraten/fragmessages.h
cowbraten/remote_commands.o: fpsgame/game.h shared/cube.h shared/tools.h
cowbraten/remote_commands.o: shared/geom.h shared/ents.h shared/command.h
cowbraten/remote_commands.o: cowbraten/colors.h shared/glexts.h
cowbraten/remote_commands.o: shared/glemu.h shared/iengine.h shared/igame.h
cowbraten/remote_commands.o: cowbraten/authservers.h cowbraten/masterserver.h
cowbraten/remote_commands.o: fpsgame/ai.h cowbraten/weaponstats_type.h
cowbraten/remote_commands.o: cowbraten/fragmessages.h
cowbraten/scoreboard.o: fpsgame/game.h shared/cube.h shared/tools.h
cowbraten/scoreboard.o: shared/geom.h shared/ents.h shared/command.h
cowbraten/scoreboard.o: cowbraten/colors.h shared/glexts.h shared/glemu.h
cowbraten/scoreboard.o: shared/iengine.h shared/igame.h
cowbraten/scoreboard.o: cowbraten/authservers.h cowbraten/masterserver.h
cowbraten/scoreboard.o: fpsgame/ai.h cowbraten/weaponstats_type.h
cowbraten/scoreboard.o: cowbraten/fragmessages.h cowbraten/weaponstats.h
cowbraten/serverbrowser.o: fpsgame/game.h shared/cube.h shared/tools.h
cowbraten/serverbrowser.o: shared/geom.h shared/ents.h shared/command.h
cowbraten/serverbrowser.o: cowbraten/colors.h shared/glexts.h shared/glemu.h
cowbraten/serverbrowser.o: shared/iengine.h shared/igame.h
cowbraten/serverbrowser.o: cowbraten/authservers.h cowbraten/masterserver.h
cowbraten/serverbrowser.o: fpsgame/ai.h cowbraten/weaponstats_type.h
cowbraten/serverbrowser.o: cowbraten/fragmessages.h cowbraten/serverbrowser.h
cowbraten/serverbrowser.o: engine/engine.h engine/world.h engine/octa.h
cowbraten/serverbrowser.o: engine/lightmap.h engine/bih.h engine/texture.h
cowbraten/serverbrowser.o: engine/model.h
cowbraten/version.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
cowbraten/version.o: shared/command.h cowbraten/colors.h shared/glexts.h
cowbraten/version.o: shared/glemu.h shared/iengine.h shared/igame.h
cowbraten/version.o: cowbraten/authservers.h cowbraten/masterserver.h
cowbraten/weaponstats.o: fpsgame/game.h shared/cube.h shared/tools.h
cowbraten/weaponstats.o: shared/geom.h shared/ents.h shared/command.h
cowbraten/weaponstats.o: cowbraten/colors.h shared/glexts.h shared/glemu.h
cowbraten/weaponstats.o: shared/iengine.h shared/igame.h
cowbraten/weaponstats.o: cowbraten/authservers.h cowbraten/masterserver.h
cowbraten/weaponstats.o: fpsgame/ai.h cowbraten/weaponstats_type.h
cowbraten/weaponstats.o: cowbraten/fragmessages.h cowbraten/weaponstats.h

shared/crypto-standalone.o: shared/cube.h shared/tools.h shared/geom.h
shared/crypto-standalone.o: shared/ents.h shared/command.h cowbraten/colors.h
shared/crypto-standalone.o: shared/iengine.h shared/igame.h
shared/crypto-standalone.o: cowbraten/authservers.h cowbraten/masterserver.h
shared/stream-standalone.o: shared/cube.h shared/tools.h shared/geom.h
shared/stream-standalone.o: shared/ents.h shared/command.h cowbraten/colors.h
shared/stream-standalone.o: shared/iengine.h shared/igame.h
shared/stream-standalone.o: cowbraten/authservers.h cowbraten/masterserver.h
shared/tools-standalone.o: shared/cube.h shared/tools.h shared/geom.h
shared/tools-standalone.o: shared/ents.h shared/command.h cowbraten/colors.h
shared/tools-standalone.o: shared/iengine.h shared/igame.h
shared/tools-standalone.o: cowbraten/authservers.h cowbraten/masterserver.h
engine/command-standalone.o: engine/engine.h shared/cube.h shared/tools.h
engine/command-standalone.o: shared/geom.h shared/ents.h shared/command.h
engine/command-standalone.o: cowbraten/colors.h shared/iengine.h
engine/command-standalone.o: shared/igame.h cowbraten/authservers.h
engine/command-standalone.o: cowbraten/masterserver.h engine/world.h
engine/server-standalone.o: engine/engine.h shared/cube.h shared/tools.h
engine/server-standalone.o: shared/geom.h shared/ents.h shared/command.h
engine/server-standalone.o: cowbraten/colors.h shared/iengine.h
engine/server-standalone.o: shared/igame.h cowbraten/authservers.h
engine/server-standalone.o: cowbraten/masterserver.h engine/world.h
engine/worldio-standalone.o: engine/engine.h shared/cube.h shared/tools.h
engine/worldio-standalone.o: shared/geom.h shared/ents.h shared/command.h
engine/worldio-standalone.o: cowbraten/colors.h shared/iengine.h
engine/worldio-standalone.o: shared/igame.h cowbraten/authservers.h
engine/worldio-standalone.o: cowbraten/masterserver.h engine/world.h
fpsgame/entities-standalone.o: fpsgame/game.h shared/cube.h shared/tools.h
fpsgame/entities-standalone.o: shared/geom.h shared/ents.h shared/command.h
fpsgame/entities-standalone.o: cowbraten/colors.h shared/iengine.h
fpsgame/entities-standalone.o: shared/igame.h cowbraten/authservers.h
fpsgame/entities-standalone.o: cowbraten/masterserver.h fpsgame/ai.h
fpsgame/entities-standalone.o: cowbraten/weaponstats_type.h
fpsgame/entities-standalone.o: cowbraten/fragmessages.h
fpsgame/server-standalone.o: fpsgame/game.h shared/cube.h shared/tools.h
fpsgame/server-standalone.o: shared/geom.h shared/ents.h shared/command.h
fpsgame/server-standalone.o: cowbraten/colors.h shared/iengine.h
fpsgame/server-standalone.o: shared/igame.h cowbraten/authservers.h
fpsgame/server-standalone.o: cowbraten/masterserver.h fpsgame/ai.h
fpsgame/server-standalone.o: cowbraten/weaponstats_type.h
fpsgame/server-standalone.o: cowbraten/fragmessages.h fpsgame/capture.h
fpsgame/server-standalone.o: fpsgame/ctf.h fpsgame/collect.h
fpsgame/server-standalone.o: fpsgame/extinfo.h fpsgame/aiman.h
cowbraten/bans-standalone.o: fpsgame/game.h shared/cube.h shared/tools.h
cowbraten/bans-standalone.o: shared/geom.h shared/ents.h shared/command.h
cowbraten/bans-standalone.o: cowbraten/colors.h shared/iengine.h
cowbraten/bans-standalone.o: shared/igame.h cowbraten/authservers.h
cowbraten/bans-standalone.o: cowbraten/masterserver.h fpsgame/ai.h
cowbraten/bans-standalone.o: cowbraten/weaponstats_type.h
cowbraten/bans-standalone.o: cowbraten/fragmessages.h
cowbraten/demo_info_message-standalone.o: fpsgame/game.h shared/cube.h
cowbraten/demo_info_message-standalone.o: shared/tools.h shared/geom.h
cowbraten/demo_info_message-standalone.o: shared/ents.h shared/command.h
cowbraten/demo_info_message-standalone.o: cowbraten/colors.h shared/iengine.h
cowbraten/demo_info_message-standalone.o: shared/igame.h
cowbraten/demo_info_message-standalone.o: cowbraten/authservers.h
cowbraten/demo_info_message-standalone.o: cowbraten/masterserver.h
cowbraten/demo_info_message-standalone.o: fpsgame/ai.h
cowbraten/demo_info_message-standalone.o: cowbraten/weaponstats_type.h
cowbraten/demo_info_message-standalone.o: cowbraten/fragmessages.h
cowbraten/managed_games-standalone.o: fpsgame/game.h shared/cube.h
cowbraten/managed_games-standalone.o: shared/tools.h shared/geom.h
cowbraten/managed_games-standalone.o: shared/ents.h shared/command.h
cowbraten/managed_games-standalone.o: cowbraten/colors.h shared/iengine.h
cowbraten/managed_games-standalone.o: shared/igame.h cowbraten/authservers.h
cowbraten/managed_games-standalone.o: cowbraten/masterserver.h fpsgame/ai.h
cowbraten/managed_games-standalone.o: cowbraten/weaponstats_type.h
cowbraten/managed_games-standalone.o: cowbraten/fragmessages.h
cowbraten/proxy_real_ip-standalone.o: fpsgame/game.h shared/cube.h
cowbraten/proxy_real_ip-standalone.o: shared/tools.h shared/geom.h
cowbraten/proxy_real_ip-standalone.o: shared/ents.h shared/command.h
cowbraten/proxy_real_ip-standalone.o: cowbraten/colors.h shared/iengine.h
cowbraten/proxy_real_ip-standalone.o: shared/igame.h cowbraten/authservers.h
cowbraten/proxy_real_ip-standalone.o: cowbraten/masterserver.h fpsgame/ai.h
cowbraten/proxy_real_ip-standalone.o: cowbraten/weaponstats_type.h
cowbraten/proxy_real_ip-standalone.o: cowbraten/fragmessages.h
cowbraten/remote_commands-standalone.o: fpsgame/game.h shared/cube.h
cowbraten/remote_commands-standalone.o: shared/tools.h shared/geom.h
cowbraten/remote_commands-standalone.o: shared/ents.h shared/command.h
cowbraten/remote_commands-standalone.o: cowbraten/colors.h shared/iengine.h
cowbraten/remote_commands-standalone.o: shared/igame.h
cowbraten/remote_commands-standalone.o: cowbraten/authservers.h
cowbraten/remote_commands-standalone.o: cowbraten/masterserver.h fpsgame/ai.h
cowbraten/remote_commands-standalone.o: cowbraten/weaponstats_type.h
cowbraten/remote_commands-standalone.o: cowbraten/fragmessages.h
cowbraten/version-standalone.o: shared/cube.h shared/tools.h shared/geom.h
cowbraten/version-standalone.o: shared/ents.h shared/command.h
cowbraten/version-standalone.o: cowbraten/colors.h shared/iengine.h
cowbraten/version-standalone.o: shared/igame.h cowbraten/authservers.h
cowbraten/version-standalone.o: cowbraten/masterserver.h
