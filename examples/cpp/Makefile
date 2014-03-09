PROGRAMS = simple

CFLAGS = -O3 -Wall -Ilib -Wno-tautological-constant-out-of-range-compare
LFLAGS = -lm -lstdc++

LIB_SOURCES = lib/effect.cpp lib/opcclient.cpp

all: $(PROGRAMS)

.cpp:
	$(CC) $(CFLAGS) $< -o $@ $(LFLAGS) $(LIB_SOURCES)

.PHONY: clean all

clean:
	rm -f $(PROGRAMS)
