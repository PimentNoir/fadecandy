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

# Bootloader to include in whole-chip .hex image
FCBOOT_IMAGE = ../bin/fc-boot-v101.hex

#######################################################

# The name of your project (used to name the compiled .dfu file)
TARGET = fc-firmware

# Temporary .hex file for application image only, without bootloader
APP_HEX = app-image.hex

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
CPPFLAGS = -Wall -Wno-sign-compare -Wno-strict-aliasing -g -Os -mcpu=cortex-m4 \
	-mthumb -nostdlib -MMD -Werror $(OPTIONS) $(INCLUDES)

# compiler options for C++ only
CXXFLAGS = -std=gnu++0x -felide-constructors -fno-exceptions -fno-rtti

# compiler options for C only
CFLAGS =

# linker script
LDSCRIPT = fcb-app.ld

# linker options
LDFLAGS = -Os -Wl,--gc-sections -mcpu=cortex-m4 -mthumb -nostdlib -T$(LDSCRIPT)

# additional libraries to link
LIBS = -lm

OBJS := $(C_FILES:.c=.o) $(CPP_FILES:.cpp=.o)

all: $(TARGET).dfu $(TARGET).hex

$(TARGET).elf: $(OBJS) $(LDSCRIPT)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

%.dfu: %.elf
	$(SIZE) $<
	$(OBJCOPY) -O binary $< $@
	$(DFU_SUFFIX) -a $@

%.hex: %.elf
	$(OBJCOPY) -O ihex $< $(APP_HEX)
	(grep -v :00000001FF $(FCBOOT_IMAGE); cat $(APP_HEX)) > $@

install: $(TARGET).dfu
	$(DFU_UTIL) -d 1d50 -D $<

benchmark: install
	python benchmark.py

# compiler generated dependency info
-include $(OBJS:.o=.d)

clean:
	rm -f *.d *.o $(TARGET).elf $(TARGET).dfu $(APP_HEX)

disassemble: $(TARGET).elf
	$(OBJDUMP) -d $< | less

symbols: $(TARGET).elf
	$(OBJDUMP) -t $< | sort | less

.PHONY: all clean install disassemble symbols benchmark
