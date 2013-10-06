/*
 * Serial pass-through adapter.
 *
 * This bridges the Teensy's USB serial port to the Fadecandy board's serial
 * interface at 9600 baud. When the green button is held, we implement a simple
 * serial port loopback which will put the Fadecandy into bootloader mode.
 */

const unsigned rxPin = 0;
const unsigned txPin = 1;
const unsigned buttonPin = 2;
const unsigned ledPin = 13;

HardwareSerial Uart = HardwareSerial();

void setup()
{
	pinMode(ledPin, OUTPUT);
	pinMode(buttonPin, INPUT_PULLUP);
	Serial.begin(9600);
}

void passthroughMode()
{
	pinMode(rxPin, INPUT);
	pinMode(txPin, OUTPUT);
	digitalWrite(ledPin, HIGH);

	// Input-to-output latency must be less than one character for the bootloader to detect this.
	while (digitalRead(buttonPin) == LOW) {
		digitalWrite(txPin, digitalRead(rxPin));
	}

	digitalWrite(ledPin, LOW);
}

void loopbackMode()
{
	Uart.begin(9600);

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