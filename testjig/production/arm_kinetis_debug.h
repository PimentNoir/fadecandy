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
    // Hold the processor core in reset, and initialize peripherals
    bool startup();

    // Flash mass-erase operation. Works even on protected devices.
    bool flashMassErase();

protected:
    // MDM-AP registers
    enum MDMReg {
        MDM_STATUS  = 0x01000000,
        MDM_CONTROL = 0x01000004,
        MDM_IDR     = 0x010000FC,
    };

    // MDM-AP bits
    enum MDMBits {
        MDM_STATUS_FLASH_ERASE_ACK = 1 << 0,
        MDM_STATUS_FLASH_READY = 1 << 1,
        MDM_STATUS_SYS_SECURITY = 1 << 2,
        MDM_STATUS_SYS_NRESET = 1 << 3,
        MDM_STATUS_MASS_ERASE_ENABLE = 1 << 4,
        MDM_STATUS_CORE_HALTED = 1 << 16,

        MDM_CONTROL_MASS_ERASE = 1 << 0,
        MDM_CONTROL_DEBUG_DISABLE = 1 << 1,
        MDM_CONTROL_DEBUG_REQ = 1 << 2,
        MDM_CONTROL_SYS_RESET_REQ = 1 << 3,
        MDM_CONTROL_CORE_HOLD_RESET = 1 << 4,
    };
};
