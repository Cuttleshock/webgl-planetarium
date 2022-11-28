DEBUG_FLAGS = -DDEBUG -O0 -g
COMPILE_FLAGS = -Wall -c
INCLUDES = -I/usr/local/include
LINK_FLAGS = -Wall -g

COMPILE_FLAGS_LINUX = `sdl2-config --cflags`
INCLUDES_LINUX =
LINK_FLAGS_LINUX = -lGL `sdl2-config --libs`

SYSROOT_WIN = /usr/local/x86_64-w64-mingw32
COMPILE_FLAGS_WIN = `$(SYSROOT_WIN)/bin/sdl2-config --cflags`
INCLUDES_WIN = -I$(SYSROOT_WIN)/include
LINK_FLAGS_WIN = `$(SYSROOT_WIN)/bin/sdl2-config --libs`

COMPILE_FLAGS_WEB = -sUSE_SDL=2
INCLUDES_WEB =
LINK_FLAGS_WEB = --shell-file $(SHELL_FILE_WEB) -sUSE_SDL=2 -sFORCE_FILESYSTEM=1 -sMAX_WEBGL_VERSION=2 -sMIN_WEBGL_VERSION=2

EXE_LINUX = $(BUILD_DIR_LINUX)/main
EXE_WIN = $(BUILD_DIR_WIN)/Main.exe
EXE_WEB = $(BUILD_DIR_WEB)/main.html

SOURCES_LINUX = main glad_gl util opengl_util
SOURCES_WIN = main glad_gl util opengl_util
SOURCES_WEB = main util opengl_util
SHELL_FILE_WEB = web_shell.html
SHADERS = shaders/particles.vert shaders/particles.frag \
	shaders/update_particles.vert shaders/update_particles.frag \
	shaders/quad.vert \
	shaders/calc_particle_attractions.frag \
	shaders/fold_texture.frag \
	shaders/init_circle.vert shaders/init_circle.frag
LICENSE = LICENSE.md
.COPY_FILES = $(SHADERS) $(LICENSE)

SOURCE_DIR = src
OBJECT_DIR = objects
SHADER_DIR = shaders

BUILD_DIR_LINUX = build-linux
BUILD_DIR_WIN = build-win
BUILD_DIR_WEB = build-web

OBJECTS_LINUX = $(addsuffix .o, $(addprefix $(BUILD_DIR_LINUX)/$(OBJECT_DIR)/, $(SOURCES_LINUX)))
COPY_FILES_LINUX = $(addprefix $(BUILD_DIR_LINUX)/, $(.COPY_FILES))

OBJECTS_WIN = $(addsuffix .o, $(addprefix $(BUILD_DIR_WIN)/$(OBJECT_DIR)/, $(SOURCES_WIN)))
COPY_FILES_WIN = $(addprefix $(BUILD_DIR_WIN)/, $(.COPY_FILES))
DLLS_WIN = $(addprefix $(BUILD_DIR_WIN)/, SDL2.dll README-SDL.txt)

OBJECTS_WEB = $(addsuffix .o, $(addprefix $(BUILD_DIR_WEB)/$(OBJECT_DIR)/, $(SOURCES_WEB)))
ASSETS_DATA_WEB = $(BUILD_DIR_WEB)/assets.data
ASSETS_JS_WEB = $(BUILD_DIR_WEB)/assets.js
EM_PACKAGER := $(dir $(firstword $(shell which emcc)))tools/file_packager

dummy :
	@echo "make {debug-}[linux|win|web]"

clean :
	rm -rf $(BUILD_DIR_LINUX)
	rm -rf $(BUILD_DIR_WIN)
	rm -rf $(BUILD_DIR_WEB)

debug-linux : COMPILE_FLAGS += $(DEBUG_FLAGS)
debug-linux : linux

debug-win : COMPILE_FLAGS += $(DEBUG_FLAGS)
debug-win : win

debug-web : COMPILE_FLAGS += $(DEBUG_FLAGS)
debug-web : web

linux : COMPILE_FLAGS += $(COMPILE_FLAGS_LINUX)
linux : INCLUDES += $(INCLUDES_LINUX)
linux : LINK_FLAGS += $(LINK_FLAGS_LINUX)

win : COMPILE_FLAGS += $(COMPILE_FLAGS_WIN)
win : INCLUDES += $(INCLUDES_WIN)
win : LINK_FLAGS += $(LINK_FLAGS_WIN)

web : COMPILE_FLAGS += $(COMPILE_FLAGS_WEB)
web : INCLUDES += $(INCLUDES_WEB)
web : LINK_FLAGS += $(LINK_FLAGS_WEB)

linux : $(EXE_LINUX) $(COPY_FILES_LINUX)

$(EXE_LINUX) : $(OBJECTS_LINUX)
	gcc $^ -o $@ $(LINK_FLAGS)

$(OBJECTS_LINUX) : $(BUILD_DIR_LINUX)/$(OBJECT_DIR)/%.o : $(SOURCE_DIR)/%.c
	mkdir -p $(@D)
	gcc $< -o $@ $(COMPILE_FLAGS) $(INCLUDES)

$(COPY_FILES_LINUX) : $(BUILD_DIR_LINUX)/% : %
	mkdir -p $(@D)
	cp $< $@

win : $(EXE_WIN) $(COPY_FILES_WIN) $(DLLS_WIN)

$(EXE_WIN) : $(OBJECTS_WIN)
	x86_64-w64-mingw32-gcc $^ -o $@ $(LINK_FLAGS)

$(OBJECTS_WIN) : $(BUILD_DIR_WIN)/$(OBJECT_DIR)/%.o : $(SOURCE_DIR)/%.c
	mkdir -p $(@D)
	x86_64-w64-mingw32-gcc $< -o $@ $(COMPILE_FLAGS) $(INCLUDES)

$(COPY_FILES_WIN) : $(BUILD_DIR_WIN)/% : %
	mkdir -p $(@D)
	cp $< $@

$(DLLS_WIN) : $(BUILD_DIR_WIN)/% : $(SYSROOT_WIN)/bin/%
	mkdir -p $(@D)
	cp $< $@

web : $(EXE_WEB) $(BUILD_DIR_WEB)/$(LICENSE) $(ASSETS_DATA_WEB) $(ASSETS_JS_WEB)

$(EXE_WEB) : $(OBJECTS_WEB) $(SHELL_FILE_WEB)
	emcc $(OBJECTS_WEB) -o $@ $(LINK_FLAGS)

$(OBJECTS_WEB) : $(BUILD_DIR_WEB)/$(OBJECT_DIR)/%.o : $(SOURCE_DIR)/%.c
	mkdir -p $(@D)
	emcc $< -o $@ $(COMPILE_FLAGS) $(INCLUDES)

$(BUILD_DIR_WEB)/$(LICENSE) : $(LICENSE)
	mkdir -p $(@D)
	cp $< $@

$(ASSETS_DATA_WEB) $(ASSETS_JS_WEB) &: $(SHADERS)
	$(EM_PACKAGER) $(ASSETS_DATA_WEB) --preload $(SHADER_DIR) --js-output=$(ASSETS_JS_WEB)
	touch $(ASSETS_JS_WEB)

.PHONY : debug-linux debug-win debug-web linux win web dummy clean
