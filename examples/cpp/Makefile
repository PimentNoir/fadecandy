PROGRAMS = simple rings spokes dot particle_trail mixer

# Important optimization options
CFLAGS = -O3 -ffast-math

# Standard libraries
LFLAGS = -lm -lstdc++ -lpthread

# Debugging
CFLAGS += -g -Wall
LFLAGS += -g

# Annoying warnings on by default on Mac OS
CFLAGS += -Wno-tautological-constant-out-of-range-compare -Wno-gnu-static-float-init

# Disable exceptions and RTTI
CFLAGS += -fno-exceptions -fno-rtti


all: $(PROGRAMS)

.cpp:
	$(CC) $(CFLAGS) $< -o $@ $(LFLAGS)

.PHONY: clean all

clean:
	rm -f $(PROGRAMS)
