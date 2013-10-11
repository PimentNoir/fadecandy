/*
 * Serial pass-through adapter.
 *
 * This bridges the Teensy's USB serial port to the Fadecandy board's serial
 * interface at 115200 baud. When the green button is held, we implement a simple
 * serial port loopback which will put the Fadecandy into bootloader mode.
 */

const unsigned rxPin = 0;
const unsigned txPin = 1;
const unsigned buttonPin = 2;
const unsigned ledPin = 13;

HardwareSerial Uart = HardwareSerial();

#define BAUD 115200

void setup()
{
    pinMode(ledPin, OUTPUT);
    pinMode(buttonPin, INPUT_PULLUP);
    Serial.begin(BAUD);
}

void passthroughMode()
{
    pinMode(rxPin, INPUT);
    pinMode(txPin, OUTPUT);
    digitalWrite(ledPin, HIGH);

    // This is only slow 9600 baud, but input-to-output latency must be less
    // than one character for the bootloader to detect the loopback.

    while (digitalRead(buttonPin) == LOW) {
        digitalWrite(txPin, digitalRead(rxPin));
    }

    digitalWrite(ledPin, LOW);
}

void loopbackMode()
{
    Uart.begin(BAUD);

    while (digitalRead(buttonPin) == HIGH) {
        if (Serial.available()) {
            Uart.write(Serial.read());
        }
        if (Uart.available()) {
            Serial.write(Uart.read());
        }
    }

    Uart.end();
}

void loop()
{
    loopbackMode();
    passthroughMode();
}