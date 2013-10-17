/*
 * Fadecandy testjig firmware for production.
 * This loads an initial bootloader/firmware image on the device, and runs electrical tests.
 *
 * Communicates with the PC for debug purposes using USB-Serial.
 * Final OK / Error status shows up on WS2811 LEDs attached to the DUT.
 */

#include "arm_kinetis_debug.h"
#include "arm_kinetis_reg.h"
#include "fc_remote.h"
#include "electrical_test.h"
#include "testjig.h"

ARMKinetisDebug target(swclkPin, swdioPin);
FcRemote remote(target);
ElectricalTest etest(target);

void setup()
{
    pinMode(ledPin, OUTPUT);
    pinMode(buttonPin, INPUT_PULLUP);
    analogReference(INTERNAL);
    Serial.begin(115200);
}

void waitForButton()
{
    // Wait for button press, with debounce

    Serial.println("");
    Serial.println("--------------------------------------------");
    Serial.println(" Fadecandy Test Jig : Press button to start");
    Serial.println("--------------------------------------------");
    Serial.println("");

    while (digitalRead(buttonPin) == LOW);
    delay(20);
    while (digitalRead(buttonPin) == HIGH) {
        // While we're waiting, blink the LED to indicate we're alive
        digitalWrite(ledPin, (millis() % 1000) < 150);
    }
    digitalWrite(ledPin, HIGH);
}

void success()
{
    Serial.println("");
    Serial.println("#### Tests Passed! ####");
    Serial.println("");

    // Green and blue throbbing means success!
    while (1) {
        float x = sin(millis() * 0.008);
        if (!remote.setPixel(0, 0, 0xc + 0x10 * x, 0)) return;
        if (!remote.setPixel(1, 0, 0, 0xc - 0x10 * x)) return;
    }
}

void loop()
{
    // Keep target power supply off when we're not using it
    etest.powerOff();

    // Button press starts the test
    waitForButton();

    // Turn on the target power supply
    if (!etest.powerOn())
        return;

    // Start debugging the target
    if (!target.begin())
        return;
    if (!target.startup())
        return;

    // Run an electrical test, to verify that the target board is okay
    if (!etest.runAll())
        return;

    // Program firmware, blinking both LEDs in unison for status.
    if (!remote.installFirmware())
        return;

    // Boot the target
    if (!remote.boot())
        return;
    
    // Disable interpolation, since we only update fbNext
    if (!remote.setFlags(CFLAG_NO_INTERPOLATION))
        return;

    // Set a default color lookup table
    if (!remote.initLUT())
        return;

    // Pixel pattern to display while running the frame rate test (white / green)
    if (!remote.setPixel(0, 16, 16, 16)) return;
    if (!remote.setPixel(1, 0, 24, 0)) return;

    // Check the frame rate; make sure the firmware is going fast enough
    if (!remote.testFrameRate())
        return;

    success();
}
