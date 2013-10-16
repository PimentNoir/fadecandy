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

#include <Arduino.h>
#include "electrical_test.h"


float ElectricalTest::analogVolts(int pin)
{
    // Analog input and voltage divider constants
    const float reference = 1.2;
    const float dividerA = 1000;    // Input to ground
    const float dividerB = 6800;    // Input to signal
    const int adcMax = 1023;

    const float scale = (reference / adcMax) * ((dividerA + dividerB) / dividerA);
    return analogRead(pin) * scale;
}

bool ElectricalTest::analogThreshold(int pin, float nominal, float tolerance)
{
    // Measure an analog input, and verify it's close enough to expected values.

    float volts = analogVolts(pin);
    float lower = nominal - tolerance;
    float upper = nominal + tolerance;

    if (volts < lower || volts > upper) {
        target.log(target.LOG_ERROR,
                "ETEST: Analog value %d outside reference range! "
                "value = %.2fv, ref = %.2fv +/- %.2fv",
                pin, volts, nominal, tolerance);
        return false;
    }

    return true;
}

bool ElectricalTest::testOutputPattern(uint8_t bits)
{
    // Set the target's 8-bit output port to the given value, and check all analog values

    // Write the port all at once
    target.digitalWritePort(outPin(0), bits);

    // Check power supply each time
    if (!analogThreshold(8, 3.3)) return false;
    if (!analogThreshold(9, 5.0)) return false;

    // Check all data signal levels
    for (unsigned n = 0; n < 8; n++) {
        bool bit = (bits >> n) & 1;
        if (!analogThreshold(n, bit ? 5.0 : 0.0))
            return false;
    }

    return true;
}

bool ElectricalTest::testAllOutputPatterns()
{
    // Multiple output patterns

    // All on, all off
    if (!testOutputPattern(0x00)) return false;
    if (!testOutputPattern(0xFF)) return false;

    // One bit set
    for (unsigned n = 0; n < 8; n++) {
        if (!testOutputPattern(1 << n))
            return false;
    }

    // One bit missing
    for (unsigned n = 0; n < 8; n++) {
        if (!testOutputPattern(0xFF ^ (1 << n)))
            return false;
    }

    // Leave all outputs on
    return testOutputPattern(0xFF);
}

bool ElectricalTest::initTarget()
{
    // Target setup that's needed only once per test run

    // Output pin directions
    for (unsigned n = 0; n < 8; n++) {
        if (!target.pinMode(outPin(n), OUTPUT))
            return false;
    }

    return true;
}

bool ElectricalTest::runAll()
{
    target.log(logLevel, "ETEST: Beginning electrical test");

    if (!initTarget())
        return false;

    // Output patterns
    if (!testAllOutputPatterns())
        return false;

    target.log(logLevel, "ETEST: Successfully completed electrical test");
    return true;
}
