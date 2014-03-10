PROGRAMS = simple rings spokes

CFLAGS = -O3 -Wall -Ilib -g -march=native
CFLAGS += -Wno-tautological-constant-out-of-range-compare -Wno-gnu-static-float-init
LFLAGS = -lm -lstdc++ -g

all: $(PROGRAMS)

.cpp:
	$(CC) $(CFLAGS) $< -o $@ $(LFLAGS)

.PHONY: clean all

clean:
	rm -f $(PROGRAMS)
