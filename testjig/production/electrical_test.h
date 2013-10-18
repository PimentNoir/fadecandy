/*
 * Electrical test for Fadecandy boards
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
#include "arm_kinetis_debug.h"

class ElectricalTest
{
public:
    ElectricalTest(ARMKinetisDebug &target, int logLevel = ARMDebug::LOG_NORMAL)
        : target(target), logLevel(logLevel) {}

    void powerOff();    // Turn off target power supply
    bool powerOn();     // Set target power supply to default voltage

    bool runAll();      // All normal electrical tests

private:
    ARMKinetisDebug &target;
    int logLevel;

    const int LOG_ERROR = ARMDebug::LOG_ERROR;

    // Pin number for an LED output
    int outPin(int index) {
        return target.pin(target.PTD, index);
    }

    void setPowerSupplyVoltage(float volts);
    float analogVolts(int pin);
    bool analogThresholdFromSample(float volts, int pin, float nominal, float tolerance = 0.30);
    bool analogThreshold(int pin, float nominal, float tolerance = 0.30);

    bool testOutputPattern(uint8_t bits);
    bool testAllOutputPatterns();
    bool testUSBConnections();
    bool testBoostConverter();
    bool testSerialConnections();
    bool testHighZ(int pin);
    bool testPull(int pin, bool state);
    bool initTarget();
};
