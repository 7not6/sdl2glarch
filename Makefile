target   := sdlgl2arch
sources  := src/sdl2arch.c src/audio.c src/core-libretro.c src/emu.c src/fifo.c src/input.c src/opengl.c
sources  += src/menu.c src/text.c src/utli.c src/video.c src/widgets.c src/core-options.c  
INCLUDES := -I. -I./src
CFLAGS   := -Wall -O2 -g $(INCLUDES)
LDFLAGS  := -static-libgcc
LIBS     := -lm -lGL #-mconsole
packages := sdl2 

# do not edit from here onwards
objects := $(addprefix build/,$(sources:.c=.o))
ifneq ($(packages),)
    LIBS    += $(shell pkg-config --libs-only-l $(packages))
    LDFLAGS += $(shell pkg-config --libs-only-L --libs-only-other $(packages))
    CFLAGS  += $(shell pkg-config --cflags $(packages))
endif

.PHONY: all clean

all: $(target)
clean:
	-rm -rf build
	-rm -f $(target)

$(target): Makefile $(objects)
	$(CC) $(LDFLAGS) -o $@ $(objects) $(LIBS)

build/%.o: %.c Makefile
	-mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -MMD -o $@ $<

-include $(addprefix build/,$(sources:.c=.d))

