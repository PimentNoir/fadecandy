#######################################################
# Environment setup

# toolchain
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-g++
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE = arm-none-eabi-size

#######################################################

# The name of your project (used to name the compiled .hex file)
TARGET = fc-boot

# Sources
C_FILES = \
	mk20dx128.c \
	usb_desc.c \
	usb_dev.c \
	serial.c \
	bootloader.c \
	dfu.c

CPP_FILES =

# Headers
INCLUDES = -I.

# CPPFLAGS = compiler options for C and C++.
# More aggressive size optimizations here than in the normal firmware!
CPPFLAGS = -Wall -Wno-sign-compare -Wno-strict-aliasing -g -Os \
	-ffunction-sections -fdata-sections -nostdlib \
	-mcpu=cortex-m4 -mthumb -nostdlib -MMD $(OPTIONS) $(INCLUDES) \

# compiler options for C++ only
CXXFLAGS = -std=gnu++0x -felide-constructors -fno-exceptions -fno-rtti

# compiler options for C only
CFLAGS =

# linker script
LDSCRIPT = mk20dx128.ld

# linker options
LDFLAGS = -Os -Wl,--gc-sections -mcpu=cortex-m4 -mthumb \
	-ffunction-sections -fdata-sections -nostdlib -T$(LDSCRIPT)

# additional libraries to link
LIBS = -lm

OBJS := $(C_FILES:.c=.o) $(CPP_FILES:.cpp=.o)

all: $(TARGET).hex $(TARGET).bin size

$(TARGET).elf: $(OBJS) $(LDSCRIPT)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

%.hex: %.elf
	$(OBJCOPY) -O ihex $< $@

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

# compiler generated dependency info
-include $(OBJS:.o=.d)

clean:
	rm -f *.d *.o $(TARGET).elf $(TARGET).hex $(TARGET).bin

# Install with OpenOCD. (No code protection!)
install: $(TARGET).hex
	openocd -f openocd.cfg -c "program $(TARGET).hex verify reset"

objdump: $(TARGET).elf
	$(OBJDUMP) -d $<

size: $(TARGET).elf
	$(SIZE) $<

.PHONY: all clean install objdump size
