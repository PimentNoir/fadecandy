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

#include <Arduino.h>
#include "arm_kinetis_debug.h"
#include "arm_kinetis_reg.h"


bool ARMKinetisDebug::startup()
{
    uint32_t idr;
    uint32_t status;

    // Make sure we're on a compatible chip. The MDM-AP peripheral is Freescale-specific.
    if (!apRead(MDM_IDR, idr))
        return false;
    if (idr != 0x001C0000) {
        log(LOG_ERROR, "ARMKinetisDebug: Didn't find a supported MDM-AP peripheral");
        return false;
    }

    // Reset the system, and hold the core in reset when it comes back
    if (!apWrite(MDM_CONTROL, MDM_CONTROL_SYS_RESET_REQ | MDM_CONTROL_CORE_HOLD_RESET))
        return false;
    if (!apReadPoll(MDM_STATUS, status, MDM_STATUS_SYS_NRESET, 0))
        return false;
    if (!apWrite(MDM_CONTROL, MDM_CONTROL_CORE_HOLD_RESET))
        return false;

    // Wait until the flash controller is ready & system is out of reset
    if (!apReadPoll(MDM_STATUS, status, MDM_STATUS_SYS_NRESET | MDM_STATUS_FLASH_READY, -1))
        return false;

    /*
     * The rest of system startup looks a lot like what the bootloader does. This is based
     * on the startup sequence used by Teensyduino.
     */

    // Enable peripheral clocks
    if (!memStore(REG_SIM_SCGC5, 0x00043F82))
        return false;
    if (!memStore(REG_SIM_SCGC6, REG_SIM_SCGC6_FTM0 | REG_SIM_SCGC6_FTM1 | REG_SIM_SCGC6_FTFL))
        return false;

    // Oscillator starts up in FEI mode.
    // Turn on crystal capacitors
    if (!memStore(REG_OSC0_CR, REG_OSC_SC8P | REG_OSC_SC2P))
        return false;
    // Enable osc, 8-32 MHz range, low power
    if (!memStore(REG_MCG_C2, REG_MCG_C2_RANGE0(2) | REG_MCG_C2_EREFS))
        return false;
    // Switch to crystal as clock source, FLL input = 16 MHz / 512
    if (!memStore(REG_MCG_C1, REG_MCG_C1_CLKS(2) | REG_MCG_C1_FRDIV(4)))
        return false;

    // Wait for crystal oscillator to begin
    uint32_t mcg;
    if (!memPoll(REG_MCG_S, mcg, REG_MCG_S_OSCINIT0, -1))
        return false;

    // wait for FLL to use oscillator
    if (!memPoll(REG_MCG_S, mcg, REG_MCG_S_IREFST, 0))
        return false;

    // wait for MCGOUT to use oscillator
    if (!memPoll(REG_MCG_S, mcg, REG_MCG_S_CLKST_MASK, REG_MCG_S_CLKST(2)))
        return false;

    // Now we're in FBE mode
    // config PLL input for 16 MHz Crystal / 4 = 4 MHz
    if (!memStore(REG_MCG_C5, REG_MCG_C5_PRDIV0(3)))
        return false;

    // config PLL for 96 MHz output
    if (!memStore(REG_MCG_C6, REG_MCG_C6_PLLS | REG_MCG_C6_VDIV0(0)))
        return false;

    // wait for PLL to start using xtal as its input
    if (!memPoll(REG_MCG_S, mcg, REG_MCG_S_PLLST, -1))
        return false;
    if (!memPoll(REG_MCG_S, mcg, REG_MCG_S_LOCK0, -1))
        return false;

    // Now we're in PBE mode
    // config divisors: 48 MHz core, 48 MHz bus, 24 MHz flash
    if (!memStore(REG_SIM_CLKDIV1, REG_SIM_CLKDIV1_OUTDIV1(1) | REG_SIM_CLKDIV1_OUTDIV2(1) | REG_SIM_CLKDIV1_OUTDIV4(3)))
        return false;
    // switch to PLL as clock source, FLL input = 16 MHz / 512
    if (!memStore(REG_MCG_C1, REG_MCG_C1_CLKS(0) | REG_MCG_C1_FRDIV(4)))
        return false;

    // wait for PLL clock to be used
    if (!memPoll(REG_MCG_S, mcg, MCG_S_CLKST_MASK, MCG_S_CLKST(3)))
        return false;

    // Now we're in PEE mode. Ready to go!
    return true;
}

bool ARMKinetisDebug::flashMassErase()
{
    // Erase all flash, even if some of it is protected.

    uint32_t status;
    if (!apRead(MDM_STATUS, status))
        return false;
    if (!(status & MDM_STATUS_FLASH_READY)) {
        log(LOG_ERROR, "FLASH: Flash controller not ready before mass erase");
        return false;
    }
    if ((status & MDM_STATUS_FLASH_ERASE_ACK)) {
        log(LOG_ERROR, "FLASH: Mass erase already in progress");
        return false;
    }

    log(LOG_NORMAL, "FLASH: Beginning mass erase operation");
    if (!apWrite(MDM_CONTROL, MDM_CONTROL_CORE_HOLD_RESET | MDM_CONTROL_MASS_ERASE))
        return false;

    // Wait for the mass erase to complete
    if (!apReadPoll(MDM_STATUS, status, MDM_STATUS_FLASH_ERASE_ACK, 0, 10000)) {
        log(LOG_ERROR, "FLASH: Timed out waiting for mass erase to complete");
        return false;
    }

    if (!(status & MDM_STATUS_FLASH_READY)) {
        log(LOG_ERROR, "FLASH: Flash controller not ready after mass erase");
        return false;
    }

    return true;
}
