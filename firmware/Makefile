#######################################################
# Environment setup

# toolchain
DFU_SUFFIX = dfu-suffix
DFU_UTIL = dfu-util
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-g++
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE = arm-none-eabi-size

#######################################################

# The name of your project (used to name the compiled .dfu file)
TARGET = fadecandy

# Sources
C_FILES = \
	startup.c \
	pins_teensy.c \
	usb_desc.c \
	usb_dev.c \
	usb_mem.c \
	serial1.c

CPP_FILES = \
	fadecandy.cpp \
	fc_usb.cpp \
	OctoWS2811z.cpp

# Headers
INCLUDES = -I.

# CPPFLAGS = compiler options for C and C++
CPPFLAGS = -Wall -Wno-sign-compare -Wno-strict-aliasing -g -Os -mcpu=cortex-m4 -mthumb -nostdlib -MMD $(OPTIONS) $(INCLUDES)

# compiler options for C++ only
CXXFLAGS = -std=gnu++0x -felide-constructors -fno-exceptions -fno-rtti

# compiler options for C only
CFLAGS =

# linker script
LDSCRIPT = fcb-app.ld

# linker options
LDFLAGS = -Os -Wl,--gc-sections -mcpu=cortex-m4 -mthumb -T$(LDSCRIPT)

# additional libraries to link
LIBS = -lm

OBJS := $(C_FILES:.c=.o) $(CPP_FILES:.cpp=.o)

all: $(TARGET).dfu

$(TARGET).elf: $(OBJS) $(LDSCRIPT)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

%.dfu: %.elf
	$(SIZE) $<
	$(OBJCOPY) -O binary $< $@
	$(DFU_SUFFIX) -a $@

install: $(TARGET).dfu
	$(DFU_UTIL) -d 1d50 -D $<

# compiler generated dependency info
-include $(OBJS:.o=.d)

clean:
	rm -f *.d *.o $(TARGET).elf $(TARGET).dfu

objdump: $(TARGET).elf
	$(OBJDUMP) -d $<

.PHONY: all clean install objdump
