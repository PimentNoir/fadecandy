PROGRAMS = simple rings spokes dot particle_trail mixer looper

# Important optimization options
CXXFLAGS = -O3 -ffast-math -fno-rtti

# Standard libraries
#LDFLAGS = -lm -lstdc++ -lpthread
LDFLAGS = -lm -lpthread

# Debugging
CXXFLAGS += -g -Wall
LDFLAGS += -g

CFLAGS += -std=c++11

# Annoying warnings on by default on Mac OS
CXXFLAGS += -Wno-tautological-constant-out-of-range-compare -Wno-gnu-static-float-init


all: $(PROGRAMS)

.cpp:
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

.PHONY: clean all

clean:
	rm -f $(PROGRAMS)
