###########################################################################
# Fadecandy Server

TARGET := fcserver

CPP_FILES += \
	src/main.cpp \
	src/opcsink.cpp \
	src/libusbev.cpp \
	src/usbdevice.cpp \
	src/fcdevice.cpp \
	src/enttecdmxdevice.cpp \
	src/fcserver.cpp

CLEAN_FILES += src/*.d src/*.o
CPPFLAGS += -Wno-tautological-constant-out-of-range-compare -Wno-strict-aliasing

###########################################################################
# System Support

UNAME := $(shell uname)
MINGW := $(findstring MINGW32, $(UNAME))
LIBS += -lstdc++ -lm

ifeq ($(UNAME), Darwin)
	# Mac OS X (32-bit build)
	LDFLAGS += -m32
	CPPFLAGS += -m32
endif

ifneq ("$(MINGW)", "")
	# Windows
	TARGET := $(TARGET).exe
endif

ifneq ("$(DEBUG)", "")
	# Debug build
	TARGET := debug-$(TARGET)
	CPPFLAGS += -g -DDEBUG
else
	# Optimized build	
	POSTBUILD_CMD := strip $(TARGET)
	CPPFLAGS += -Os -DNDEBUG
	LDFLAGS += -Os
endif

###########################################################################
# Built-in libev

C_FILES += libev/ev.c

CFLAGS += -DEV_STANDALONE -Wno-comment -Wno-unused-variable \
	-Wno-extern-initializer -Wno-unused-value -Wno-bitwise-op-parentheses

INCLUDES += -Ilibev
CLEAN_FILES += libev/*.d libev/*.o

###########################################################################
# Built-in rapidjson

INCLUDES += -I.

###########################################################################
# Built-in libusbx

C_FILES += \
	libusbx/libusb/core.c \
	libusbx/libusb/descriptor.c \
	libusbx/libusb/hotplug.c \
	libusbx/libusb/io.c \
	libusbx/libusb/strerror.c \
	libusbx/libusb/sync.c

ifeq ($(UNAME), Darwin)
	# Mac OS X

	C_FILES += \
		libusbx/libusb/os/darwin_usb.c \
		libusbx/libusb/os/poll_posix.c \
		libusbx/libusb/os/threads_posix.c

	LIBS += -framework CoreFoundation -framework IOKit -lobjc
	CFLAGS += -DOS_DARWIN -DTHREADS_POSIX -DPOLL_NFDS_TYPE=nfds_t \
		-DLIBUSB_CALL= -DDEFAULT_VISIBILITY= -DHAVE_GETTIMEOFDAY
endif

ifneq ("$(MINGW)", "")
	# Windows
	CPPFLAGS += -DOS_WINDOWS
endif

INCLUDES += -Ilibusbx/libusb
CLEAN_FILES += \
	libusbx/libusb/*.d libusbx/libusb/*.o \
	libusbx/libusb/os/*.d libusbx/libusb/os/*.o

###########################################################################
# Build Rules

# Compiler options for C and C++
CPPFLAGS += -MMD $(INCLUDES)

# Compiler options for C++ only
CXXFLAGS += -std=gnu++0x -felide-constructors -fno-exceptions -fno-rtti

OBJS := $(CPP_FILES:.cpp=.o) $(C_FILES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
	$(POSTBUILD_CMD)

-include $(OBJS:.o=.d)

clean:
	rm -f $(CLEAN_FILES) $(TARGET)

.PHONY: clean all
