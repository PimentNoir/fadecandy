/*
 * Fadecandy testjig firmware for production.
 * This loads an initial bootloader/firmware image on the device, and runs electrical tests.
 *
 * Communicates with the PC for debug purposes using USB-Serial.
 * Final OK / Error status shows up on WS2811 LEDs attached to the DUT.
 */

#include "arm_kinetis_debug.h"
#include "arm_kinetis_reg.h"

const unsigned buttonPin = 2;
const unsigned ledPin = 13;
const unsigned swclkPin = 3;
const unsigned swdioPin = 4;
ARMKinetisDebug target;

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
    if (!target.startup())
        return;

    /*
     * Try blinking an LED on the target
     */

    // Set PC5 as output
    if (!target.memStore(REG_PORTC_PCR5, REG_PORT_PCR_MUX(1) | REG_PORT_PCR_DSE | REG_PORT_PCR_SRE))
        return;
    if (!target.memStore(REG_GPIOC_PDDR, 1 << 5))
        return;

    while (target.memStore(REG_GPIOC_PTOR, 1 << 5)) {
        delay(100);
    }
}
