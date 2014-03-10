PROGRAMS = simple rings spokes dot particle_trail

# Important optimization options
CFLAGS = -O3  -march=native -ffast-math

# Standard libraries
LFLAGS = -lm -lstdc++

# Debugging
CFLAGS += -g -Wall -Wno-tautological-constant-out-of-range-compare -Wno-gnu-static-float-init
LFLAGS += -g

all: $(PROGRAMS)

.cpp:
	$(CC) $(CFLAGS) $< -o $@ $(LFLAGS)

.PHONY: clean all

clean:
	rm -f $(PROGRAMS)
