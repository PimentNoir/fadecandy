/*
 * Fadecandy testjig firmware for production.
 * This loads an initial bootloader/firmware image on the device, and runs electrical tests.
 *
 * Communicates with the PC for debug purposes using USB-Serial.
 * Final OK / Error status shows up on WS2811 LEDs attached to the DUT.
 */

#include "arm_kinetis_debug.h"
#include "arm_kinetis_reg.h"
#include "firmware_data.h"

#define CFLAG_NO_DITHERING      (1 << 0)
#define CFLAG_NO_INTERPOLATION  (1 << 1)
#define CFLAG_NO_ACTIVITY_LED   (1 << 2)
#define CFLAG_LED_CONTROL       (1 << 3)
#define LUT_CH_SIZE             257
#define LUT_TOTAL_SIZE          (LUT_CH_SIZE * 3)
#define PIXELS_PER_PACKET       21
#define LUTENTRIES_PER_PACKET   31
#define PACKETS_PER_FRAME       25
#define PACKETS_PER_LUT         25

const unsigned buttonPin = 2;
const unsigned ledPin = 13;
const unsigned swclkPin = 3;
const unsigned swdioPin = 4;

ARMKinetisDebug target;
const unsigned targetLedPin = target.PTC5;

void setup()
{
    pinMode(ledPin, OUTPUT);
    pinMode(buttonPin, INPUT_PULLUP);
    Serial.begin(115200);
}

bool fbInit()
{
    // Install a trivial identity-mapping LUT, writing directly to the firmware's LUT buffer.
    for (unsigned channel = 0; channel < 3; channel++) {
        for (unsigned index = 0; index < LUT_CH_SIZE; index++) {
            unsigned value = index << 8;
            if (value > 0xFFFF)
                value = 0xFFFF;
            if (!target.memStoreHalf(fw_pLUT + 2*(index + channel*LUT_CH_SIZE), value))
                return false;
        }
    }

    // Control flags: Disable interpolation so only fbNext is in use, and use manual LED control
    uint8_t control = CFLAG_NO_ACTIVITY_LED | CFLAG_NO_INTERPOLATION;
    return target.memStoreByte(fw_pFlags, control);;

    return true;
}

bool fbWrite(unsigned id, int red, int green, int blue)
{
    // Write one pixel directly into the fbNext framebuffer on the target.

    uint32_t idPacket = id / PIXELS_PER_PACKET;
    uint32_t idOffset = fw_usbPacketBufOffset + 1 + (id % PIXELS_PER_PACKET) * 3;

    uint32_t fb;        // Address of the current fcFramebuffer bound to fbNext
    uint32_t packet;    // Pointer to usb_packet in question

    return
        target.memLoad(fw_pFbNext, fb) &&
        target.memLoad(fb + idPacket*4, packet) &&
        target.memStoreByte(packet + idOffset + 0, constrain(red, 0, 255)) &&
        target.memStoreByte(packet + idOffset + 1, constrain(green, 0, 255)) &&
        target.memStoreByte(packet + idOffset + 2, constrain(blue, 0, 255));
}

bool installFirmware()
{
    bool blink = false;
    ARMKinetisDebug::FlashProgrammer programmer(target, fw_data, fw_sectorCount);

    if (!programmer.begin())
        return false;

    while (!programmer.isComplete()) {
        blink = !blink;
        if (!programmer.next()) return false;
        if (!target.pinMode(targetLedPin, OUTPUT)) return false;
        if (!target.digitalWrite(targetLedPin, blink)) return false;
        digitalWrite(ledPin, blink);
    }

    return true;
}

void loop()
{
    Serial.println("");
    Serial.println("--------------------------------------------");
    Serial.println(" Fadecandy Test Jig : Press button to start");
    Serial.println("--------------------------------------------");
    Serial.println("");

    while (digitalRead(buttonPin) == LOW);
    delay(20);   // Debounce delay
    while (digitalRead(buttonPin) == HIGH) {
        // While we're waiting, blink the LED to indicate we're alive
        digitalWrite(ledPin, (millis() % 1000) < 150);
    }
    digitalWrite(ledPin, HIGH);

    // Start debugging the target
    if (!target.begin(swclkPin, swdioPin))
        return;
    if (!target.startup())
        return;

    // Program firmware, blinking both LEDs in unison for status.
    if (!installFirmware())
        return;

    // Run the new firmware, and let it boot
    if (!target.reset())
        return;
    delay(50);

    // Initialize direct access to the firmware's framebuffer
    if (!fbInit())
        return;

    // Green and blue throbbing means success!
    while (1) {
        float x = sin(millis() * 0.008);
        if (!fbWrite(0, 0, 0xc + 0x10 * x, 0)) return;
        if (!fbWrite(1, 0, 0, 0xc - 0x10 * x)) return;
    }
}
