// Testjig hardware definitions

#pragma once

// UI
static const unsigned buttonPin = 2;
static const unsigned ledPin = 13;

// Debug port
static const unsigned swclkPin = 3;
static const unsigned swdioPin = 4;

// Electrical testing
static const unsigned fcTXPin = 0;
static const unsigned fcRXPin = 1;
static const unsigned powerPWMPin = 10;
static const unsigned usbDMinusPin = 5;
static const unsigned usbDPlusPin = 6;
static const unsigned usbShieldGroundPin = 7;
static const unsigned usbSignalGroundPin = 8;
static const unsigned analogTarget33vPin = 8;
static const unsigned analogTargetVUsbPin = 9;

// LED functional testing (fast pin)
static const unsigned dataFeedbackPin = 11;

// Analog constants
static const float powerSupplyFullScaleVoltage = 6.42;
