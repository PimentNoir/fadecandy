/*
 * Simple ARM debug interface for Arduino, using the SWD (Serial Wire Debug) port.
 * Extensions for Freescale Kinetis chips.
 * 
 * Copyright (c) 2013 Micah Elizabeth Scott
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once
#include "arm_debug.h"

class ARMKinetisDebug : public ARMDebug
{
public:
    // First-time initialization, resetting into system halt state.
    bool startup();

    // Individual parts of startup():
    bool detect();              // Detect supported Kinetis hardware
    bool reset();               // System reset
    bool debugHalt();           // Turn on debugging and enter halt state
    bool peripheralInit();      // Initialize peripherals into default state

    // Flash mass-erase operation. Works even on protected devices.
    bool flashMassErase();

    // Initialize the FlexRAM buffer for flash sector programming
    bool flashSectorBufferInit();

    // Write a chunk of data to the flash sector buffer
    bool flashSectorBufferWrite(uint32_t bufferOffset, uint32_t *data, unsigned count);

    // Write one flash sector from the buffer
    bool flashSectorProgram(uint32_t address);

    static const uint32_t FLASH_SECTOR_SIZE = 1024;

protected:
    // Low-level flash interface
    bool ftfl_busyWait();
    bool ftfl_launchCommand();
    bool ftfl_setFlexRAMFunction(uint8_t controlCode);
    bool ftfl_programSection(uint32_t address, uint32_t numLWords);
    bool ftfl_handleCommandStatus(const char *cmdSpecificError = 0);
};
