/*
 * Simple ARM debug interface for Arduino, using the SWD (Serial Wire Debug) port.
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
#include <stdint.h>
#include <stdbool.h>


class ARMDebug
{
public:
    enum LogLevel {
        LOG_NONE = 0,
        LOG_ERROR,
        LOG_NORMAL,
        LOG_TRACE_MEM,
        LOG_TRACE_AP,
        LOG_TRACE_DP,
        LOG_TRACE_SWD,
        LOG_MAX
    };

    /**
     * Reinitialize the debug interface, and identify the connected chip.
     * This resets the target chip, putting it in SWD mode and logging its
     * identity.
     *
     * Returns true on success, logs and returns false on error.
     */
    bool begin(unsigned clockPin, unsigned dataPin, LogLevel logLevel = LOG_NORMAL);

    // Memory operations (AHB bus)
    bool memStore(uint32_t addr, uint32_t data);
    bool memStore(uint32_t addr, uint32_t *data, unsigned count);
    bool memLoad(uint32_t addr, uint32_t &data);
    bool memLoad(uint32_t addr, uint32_t *data, unsigned count);

    // Write with verify
    bool memStoreAndVerify(uint32_t addr, uint32_t data);
    bool memStoreAndVerify(uint32_t addr, uint32_t *data, unsigned count);

    // Poll for an expected value
    bool memPoll(unsigned addr, uint32_t &data, uint32_t mask, uint32_t expected, unsigned retries = 32);

private:
    uint8_t clockPin, dataPin;
    LogLevel logLevel;

    // Cached versions of ARM debug registers
    struct {
        uint32_t select;
    } cache;

protected:
    // Internal logging
    void log(int level, const char *fmt, ...);

    // Low-level wire interface (LSB-first)
    void wireWrite(uint32_t data, unsigned nBits);
    uint32_t wireRead(unsigned nBits);
    void wireWriteTurnaround();
    void wireReadTurnaround();
    void wireWriteIdle();

    // Error diagnostics and recovert
    bool handleFault();

    // Packet assembly tools
    uint8_t packHeader(unsigned addr, bool APnDP, bool RnW);
    bool evenParity(uint32_t word);

    // Debug port layer
    bool dpWrite(unsigned addr, bool APnDP, uint32_t data);
    bool dpRead(unsigned addr, bool APnDP, uint32_t &data);
    bool dpSelect(unsigned addr);

    // Access port layer
    bool apWrite(unsigned addr, uint32_t data);
    bool apRead(unsigned addr, uint32_t &data);

    // Poll for an expected value
    bool dpReadPoll(unsigned addr, uint32_t &data, uint32_t mask, uint32_t expected, unsigned retries = 32);
    bool apReadPoll(unsigned addr, uint32_t &data, uint32_t mask, uint32_t expected, unsigned retries = 32);

    // Individual initialization steps (already included in begin)
    bool getIDCODE(uint32_t &idcode);
    bool debugPortPowerup();
    bool debugPortReset();
    bool initMemPort();

    // Debug port registers
    enum DebugPortReg {
        ABORT = 0x0,
        IDCODE = 0x0,
        CTRLSTAT = 0x4,
        SELECT = 0x8,
        RDBUFF = 0xC
    };

    // CTRL/STAT bits
    enum CtrlStatBit {
        CSYSPWRUPACK = 1 << 31,
        CSYSPWRUPREQ = 1 << 30,
        CDBGPWRUPACK = 1 << 29,
        CDBGPWRUPREQ = 1 << 28,
        CDBGRSTACK   = 1 << 27,
        CDBGRSTREQ   = 1 << 26
    };

    // Memory Access Port registers
    enum MemPortReg {
        MEM_CSW = 0x00,
        MEM_TAR = 0x04,
        MEM_DRW = 0x0C,
        MEM_IDR = 0xFC,
    };
};
