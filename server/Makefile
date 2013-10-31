###########################################################################
# Fadecandy Server

TARGET := fcserver

CPP_FILES += \
	src/main.cpp \
	src/opcsink.cpp \
	src/usbdevice.cpp \
	src/fcdevice.cpp \
	src/enttecdmxdevice.cpp \
	src/fcserver.cpp \
	src/version.cpp \
	src/tinythread.cpp

INCLUDES += -Isrc
CLEAN_FILES += src/*.d src/*.o
CPPFLAGS += -Wno-strict-aliasing

###########################################################################
# System Support

UNAME := $(shell uname)
MINGW := $(findstring MINGW32, $(UNAME))
LIBS += -lstdc++ -lm
VERSION := $(shell git describe --match "fcserver-*")
CXXFLAGS += -DFCSERVER_VERSION=$(VERSION)

ifeq ($(UNAME), Darwin)
	# Mac OS X (32-bit build)
	LDFLAGS += -m32
	CPPFLAGS += -m32 -Wno-tautological-constant-out-of-range-compare -DHAVE_POLL_H
endif

ifneq ("$(MINGW)", "")
	# Windows
	TARGET := $(TARGET).exe
	CPPFLAGS += -D_WIN32_WINNT=0x0501
endif

ifneq ("$(DEBUG)", "")
	# Debug build
	TARGET := debug-$(TARGET)
	CPPFLAGS += -g -DDEBUG -DENABLE_LOGGING
else
	# Optimized build	
	POSTBUILD_CMD := strip $(TARGET)
	CPPFLAGS += -Os -DNDEBUG
	LDFLAGS += -Os
endif

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
	CPPFLAGS += -DOS_DARWIN -DTHREADS_POSIX -DPOLL_NFDS_TYPE=nfds_t \
		-DLIBUSB_CALL= -DDEFAULT_VISIBILITY= -DHAVE_GETTIMEOFDAY
endif

ifneq ("$(MINGW)", "")
	# Windows

	C_FILES += \
		libusbx/libusb/os/windows_usb.c \
		libusbx/libusb/os/poll_windows.c \
		libusbx/libusb/os/threads_windows.c

	LIBS += -lws2_32
	CPPFLAGS += -DOS_WINDOWS -DPOLL_NFDS_TYPE=int -DDEFAULT_VISIBILITY= -DHAVE_GETTIMEOFDAY
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

$(TARGET): libusbx/libusb/libusb.h $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
	$(POSTBUILD_CMD)
	rm -f src/version.o

-include $(OBJS:.o=.d)

clean:
	rm -f $(CLEAN_FILES) $(TARGET)

# Git submodule setup
libusbx/libusb/libusb.h:
	git submodule init
	git submodule update

.PHONY: clean all
