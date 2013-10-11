/*
 * Fadecandy testjig firmware for production.
 * This loads an initial bootloader/firmware image on the device, and runs electrical tests.
 *
 * Communicates with the PC for debug purposes using USB-Serial.
 * Final OK / Error status shows up on WS2811 LEDs attached to the DUT.
 */

#include "arm_debug.h"

const unsigned buttonPin = 2;
const unsigned ledPin = 13;
const unsigned swclkPin = 3;
const unsigned swdioPin = 4;
ARMDebug target;

void setup()
{
    pinMode(ledPin, OUTPUT);
    pinMode(buttonPin, INPUT_PULLUP);
    Serial.begin(115200);
}

void loop()
{
    Serial.println("");
    Serial.println("--------------------------------------------");
    Serial.println(" Fadecanday Test Jig : Press button to start");
    Serial.println("--------------------------------------------");
    Serial.println("");

    while (digitalRead(buttonPin) == LOW);
    delay(20);   // Debounce delay
    while (digitalRead(buttonPin) == HIGH) {
        // While we're waiting, blink the LED to indicate we're alive
        digitalWrite(ledPin, (millis() % 1000) < 150);
    }
    digitalWrite(ledPin, HIGH);

    if (!target.begin(swclkPin, swdioPin, target.LOG_TRACE_MEM))
        return;
    if (!target.halt())
        return;
    if (!target.sysReset())
        return;

    uint32_t data;
    if (!target.memLoad(0x00000000, data))
        return;
    if (!target.memLoad(0x00000000, data))
        return;
    if (!target.memLoad(0x00000004, data))
        return;
    if (!target.memLoad(0x00000004, data))
        return;
    if (!target.memLoad(0x00000008, data))
        return;
    if (!target.memLoad(0x00000008, data))
        return;
    if (!target.memLoad(0x20000000, data))
        return;

    data ^= 0xFFFFFFFF;
    if (!target.memStore(0x20000000, data))
        return;
    if (!target.memLoad(0x20000000, data))
        return;
}
