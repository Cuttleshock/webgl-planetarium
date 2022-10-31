COMPILE_FLAGS = -Wall -c `sdl2-config --cflags`
DEBUG_FLAGS = -DDEBUG -O0 -g
INCLUDES = -I/usr/local/include
LINK_FLAGS = -Wall -g -lGL `sdl2-config --libs`

EXE = $(BUILD_DIR)/main

SOURCE_FILES = main glad_gl
.COPY_FILES = shaders/particles.vert shaders/particles.frag LICENSE.md

SOURCE_DIR = src
OBJECT_DIR = objects
SHADER_DIR = shaders
BUILD_DIR = build

OBJECTS = $(addsuffix .o, $(addprefix $(BUILD_DIR)/$(OBJECT_DIR)/, $(SOURCE_FILES)))
COPY_FILES = $(addprefix $(BUILD_DIR)/, $(.COPY_FILES))

linux : $(EXE) $(COPY_FILES)

clean :
	rm -rf $(BUILD_DIR)

debug : COMPILE_FLAGS += $(DEBUG_FLAGS)
debug : linux

$(EXE) : $(OBJECTS)
	gcc $^ -o $@ $(LINK_FLAGS)

$(OBJECTS) : $(BUILD_DIR)/$(OBJECT_DIR)/%.o : $(SOURCE_DIR)/%.c
	mkdir -p $(@D)
	gcc $< -o $@ $(COMPILE_FLAGS) $(INCLUDES)

$(COPY_FILES) : $(BUILD_DIR)/% : %
	mkdir -p $(@D)
	cp $< $@

.PHONY : debug linux clean
