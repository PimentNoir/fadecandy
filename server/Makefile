#######################################################
# Environment setup

INCLUDES = -I/usr/local/include/libusb-1.0
LIBS = -L/usr/local/lib -lstdc++ -lm

ifeq ($(BUILD_TYPE), osx_release)
	# Janky 32-bit OSX release build, using static libs.
	LIBS += /usr/local/Cellar/libusbx/1.0.16/lib/libusb-1.0.a
	LIBS += -framework CoreFoundation -framework IOKit -lobjc
	TARGET := fcserver-osx
	POSTBUILD_CMD = strip $(TARGET); otool -L $(TARGET)
	CPPFLAGS += -m32
	LDFLAGS += -m32
else
	# Generic build
	LIBS += -lusb-1.0
	TARGET := fcserver
endif

#######################################################

CPP_FILES = \
	src/main.cpp \
	src/opcsink.cpp \
	src/libusbev.cpp \
	src/usbdevice.cpp \
	src/fcdevice.cpp \
	src/enttecdmxdevice.cpp \
	src/fcserver.cpp

C_FILES = \
	libev/ev.c

INCLUDES += -I. -Ilibev

# libev configuration
CFLAGS += -DEV_STANDALONE -Wno-comment -Wno-unused-variable -Wno-extern-initializer \
	-Wno-unused-value -Wno-bitwise-op-parentheses

# CPPFLAGS = compiler options for C and C++
CPPFLAGS += -Wno-tautological-constant-out-of-range-compare -Wno-strict-aliasing \
	-g -Os -MMD $(INCLUDES)

# compiler options for C++ only
CXXFLAGS += -std=gnu++0x -felide-constructors -fno-exceptions -fno-rtti

# linker options and libraries
LDFLAGS += -Os

OBJS := $(CPP_FILES:.cpp=.o) $(C_FILES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
	$(POSTBUILD_CMD)

# compiler generated dependency info
-include $(OBJS:.o=.d)

clean:
	rm -f src/*.d src/*.o libev/*.d libev/*.o $(TARGET)

.PHONY: clean all
