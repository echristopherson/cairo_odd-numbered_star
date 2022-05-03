PROJECT		= cairo_odd-numbered_star
BINARY		= $(PROJECT)
DEBUG_FLAGS     = -g -O0
CAIRO_FLAGS     = $(shell pkg-config --libs --cflags cairo)
GTK_FLAGS       = $(shell pkg-config --libs --cflags gtk+-2.0)
ADDL_LIB_FLAGS  = -lm -D_DEFAULT_SOURCE -D_GNU_SOURCE
CFLAGS	       := $(CFLAGS) -Wall -Wextra

ifeq ($(origin NUM_POINTS), undefined)
	NUM_POINTS_RUN_ARGS   =
	NUM_POINTS_DEBUG_ARGS =
else
	NUM_POINTS_RUN_ARGS   = -n $(NUM_POINTS)
	NUM_POINTS_DEBUG_ARGS = --args -n $(NUM_POINTS)
endif

all: $(BINARY)

# TODO: Don't hardcode path
open: star.svg
	xdg-open "$<"

# TODO: Don't hardcode path
star.svg: run

clean:
	-rm $(BINARY)

run: $(BINARY)
	chmod +x $<
	./$< $(NUM_POINTS_ARGS)

debug: $(BINARY)
	chmod +x $<
	gdb ./$< $(NUM_POINTS_ARGS)

$(BINARY): $(PROJECT).c
	$(CC) $^ -o $@    $(CFLAGS) $(DEBUG_FLAGS) $(CAIRO_FLAGS) $(GTK_FLAGS) $(ADDL_LIB_FLAGS)
