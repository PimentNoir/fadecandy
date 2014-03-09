PROGRAMS = simple

CFLAGS = -O3 -Wall -Ilib -Wno-tautological-constant-out-of-range-compare
LFLAGS = -lm -lstdc++

all: $(PROGRAMS)

.cpp:
	$(CC) $(CFLAGS) $< -o $@ $(LFLAGS)

.PHONY: clean all

clean:
	rm -f $(PROGRAMS)
