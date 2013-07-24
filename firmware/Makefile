#######################################################
# Environment setup

# Teensy loader tools (for 'make install')
TOOLSPATH = /Applications/Arduino.app/Contents/Resources/Java/hardware/tools

# toolchain
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-g++
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE = arm-none-eabi-size

#######################################################

# The name of your project (used to name the compiled .hex file)
TARGET = fadecandy

# configurable options
OPTIONS = -DF_CPU=48000000

# Sources
C_FILES = \
	mk20dx128.c \
	pins_teensy.c \
	usb_desc.c \
	usb_dev.c \
	usb_mem.c

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
LDSCRIPT = mk20dx128.ld

# linker options
LDFLAGS = -Os -Wl,--gc-sections -mcpu=cortex-m4 -mthumb -T$(LDSCRIPT)

# additional libraries to link
LIBS = -lm

OBJS := $(C_FILES:.c=.o) $(CPP_FILES:.cpp=.o)

all: $(TARGET).hex

$(TARGET).elf: $(OBJS) $(LDSCRIPT)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

%.hex: %.elf
	$(SIZE) $<
	$(OBJCOPY) -O ihex -R .eeprom $< $@

# compiler generated dependency info
-include $(OBJS:.o=.d)

clean:
	rm -f *.d *.o $(TARGET).elf $(TARGET).hex

install: $(TARGET).hex
	$(abspath $(TOOLSPATH))/teensy_post_compile -file=$(TARGET) -path=$(shell pwd) -tools=$(abspath $(TOOLSPATH))
	$(abspath $(TOOLSPATH))/teensy_reboot

objdump: $(TARGET).elf
	$(OBJDUMP) -d $<

.PHONY: all clean install objdump
