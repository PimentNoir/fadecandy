Fadecandy: USB Protocol
=======================

To achieve the best CPU efficiency, Fadecandy uses a custom packet-oriented USB protocol rather than emulating a USB serial device. This simple USB protocol is easy to speak using cross-platform libraries like [libusb](http://www.libusb.org) and [PyUSB](http://pyusb.sourceforge.net/). Examples are included.

If you use the included `fcserver` daemon, you need not worry about the USB protocol at all.

Device Information
------------------

Attribute       | Value
--------------- | -----
Vendor ID       | 0x1d50
Product ID      | 0x607a
Manufacturer    | "scanlime"
Product         | "Fadecandy"
Serial          | Unique ID string
Device Class    | Vendor-specific
Device Version  | See below
Configurations  | 1
Endpoints       | 1
Endpoint 1      | Bulk OUT (Host to Device), 64-byte packets

The Fadecandy device has one USB Configuration with two Interfaces:

* **Interface #0** is a vendor-specific interface with one Bulk OUT endpoint for communicating control data and video keyframes.
* **Interface #1** is compatible with the USB Device Firmware Update spec. It's used during firmware upgrade to ask the device to enter bootloader mode. During normal operation, it's safe to ignore this interface.

Some device version codes may be used to indicate experimental or unofficial firmware. Currently, versions in the range 0x0300 through 0x03FF are reserved for unofficial Fadecandy firmware forks.

Getting Started
---------------

To use the Fadecandy Controller's USB protocol directly, you'll need to:

1. **Search** for the device you're interested in. You can look for all Fadecandy devices by looking at the Vendor and Product IDs. You can find a specific Fadecandy device by looking at its USB serial number string.
3. **Open** the device, and set its configuration. Depending on the operating system, you may also need to claim interface #0 so your application has exclusive access to it.
4. Set up a **Color Look-Up Table**. There is no default table, the firmware expects you to calculate one based on the gamma and whitepoint you'd like to use.
5. Send **Video Frames** at any time. When you complete a frame, the firmware immediately begins interpolating to it.

There's a [minimal example](https://github.com/scanlime/fadecandy/blob/master/examples/python/usb-lowlevel.py) written in Python using PyUSB. It calculates a simple Color LUT and sends random video frames.

To compare with the Color LUT calculation used by `fcserver`, see the [FCDevice::writeColorCorrection](https://github.com/scanlime/fadecandy/blob/47a4e551a71444923a1e6754f29f424e28a62090/server/src/fcdevice.cpp#L233) function in the fcserver source code.


Bulk Endpoint
-------------

The device has a single Bulk OUT endpoint which expects packets of up to 64 bytes. Multiple packets may be transmitted in one LibUSB "write" operation, as long as the buffer you provide is a multiple of 64 bytes in length.

Each packet begins with an 8-bit control byte, which is divided into three bit-fields:

Bits 7..6  | Bit 5       | Bits 4..0
---------- | ----------- | ------------
Type code  | 'Final' bit | Packet index

* The 'type' code indicates what kind of packet this is.
* The 'final' bit, if set, causes the most recent group of packets to take effect
* The packet index is used to sequence packets within a particular type code

The following packet types are recognized:

Type code | Meaning of 'final' bit          | Index range | Packet contents
--------- | ------------------------------- | ----------- | -------------------------------------
0         | Interpolate to new video frame  | 0 … 24      | Up to 21 pixels, 24-bit RGB
1         | Instantly apply new color LUT   | 0 … 24      | Up to 31 16-bit lookup table entries
2         | (reserved)                      | 0           | Set configuration data
3         |                                 |             | (reserved)

Video Packets
-------------

In a type 0 packet, the USB packet contains up to 21 pixels of 24-bit RGB color data. The last packet (index 24) only needs to contain 8 valid pixels. Pixels 9-20 in these packets are ignored.

Byte Offset   | Description
------------- | ------------
0             | Control byte
1             | Pixel 0, Red
2             | Pixel 0, Green
3             | Pixel 0, Blue
4             | Pixel 1, Red
5             | Pixel 1, Green
6             | Pixel 1, Blue
…             | …
61            | Pixel 20, Red
62            | Pixel 20, Green
63            | Pixel 20, Blue

Color LUT Packets
-----------------

In a type 1 packet, the USB packet contains up to 31 lookup-table entries. The lookup table is structured as three arrays of 257 entries, starting with the entire red-channel LUT, then the green-channel LUT, then the blue-channel LUT. Each packet is structured as follows:

Byte Offset   | Description
------------- | ------------
0             | Control byte
1             | Reserved (0)
2             | LUT entry #0, low byte
3             | LUT entry #0, high byte
4             | LUT entry #1, low byte
5             | LUT entry #1, high byte
…             | …
62            | LUT entry #30, low byte
63            | LUT entry #30, high byte

Configuration Packet
--------------------

A type 2 packet sets optional device-wide configuration settings:

Byte Offset | Bits   | Description
----------- | ------ | ------------
0           | 7 … 0  | Control byte
1           | 7 … 5  | (reserved)
1           | 4      | 0 = Normal mode, 1 = Reserved operation mode
1           | 3      | Manual LED control bit
1           | 2      | 0 = LED shows USB activity, 1 = LED under manual control
1           | 1      | Disable keyframe interpolation
1           | 0      | Disable dithering
2 … 63      | 7 … 0  | (reserved)

The "reserved operation mode" may be used by unofficial Fadecandy firmware that includes experimental or application-specific effects. This reserved bit is guaranteed not to be used during normal operation by future versions of fcserver.

Control Requests
----------------

In addition to the OUT endpoint, the device also supports vendor-specific control requests:

bmRequestType | bRequest | wValue | wIndex | wLength | Description
------------- | -------- | ------ | ------ | ------- | ---------------------------------------------
0xC0          | 0x01     | 0      | 0      | 4       | Read rendered frame counter (32-bit, little endian)
0xC0          | 0x01     | 0      | 1      | 4       | Read received keyframe counter (32-bit, little endian)
0xC0          | 0x7E     | x      | 4      | x       | Read Microsoft WCID descriptor
0xC0          | 0x7E     | x      | 5      | x       | Read Microsoft Extended Properties descriptor

USB Descriptors
---------------

For reference, this is sample output from the Mac OS X [USB Prober](https://developer.apple.com/library/mac/qa/qa1370/_index.html) diagnostics tool:

	Full Speed device @ 15 (0x14200000): Composite device: "Fadecandy"
	    Port Information:   0x101a
	           Not Captive
	           Attached to Root Hub
	           External Device
	           Connected
	           Enabled
	           Connected to External Port
	    Number Of Endpoints (includes EP0):   
	        Total Endpoints for Configuration 1 (current):   2
	    Device Descriptor   
	        Descriptor Version Number:   0x0200
	        Device Class:   0   (Composite)
	        Device Subclass:   0
	        Device Protocol:   0
	        Device MaxPacketSize:   64
	        Device VendorID/ProductID:   0x1D50/0x607A   (unknown vendor)
	        Device Version Number:   0x0108
	        Number of Configurations:   1
	        Manufacturer String:   1 "scanlime"
	        Product String:   2 "Fadecandy"
	        Serial Number String:   3 "ENICCULVLDQJQDWD"
	    Configuration Descriptor (current config)   
	        Length (and contents):   43
	            Raw Descriptor (hex)    0000: 09 02 2B 00 02 01 00 80  32 09 04 00 00 01 FF 00  
	            Raw Descriptor (hex)    0010: 00 00 07 05 01 02 40 00  00 09 04 01 00 00 FE 01  
	            Raw Descriptor (hex)    0020: 01 04 09 21 0D 10 27 00  04 01 01 
	        Number of Interfaces:   2
	        Configuration Value:   1
	        Attributes:   0x80 (bus-powered)
	        MaxPower:   100 mA
	        Interface #0 - Vendor-specific   
	            Alternate Setting   0
	            Number of Endpoints   1
	            Interface Class:   255   (Vendor-specific)
	            Interface Subclass;   0   (Vendor-specific)
	            Interface Protocol:   0
	            Endpoint 0x01 - Bulk Output   
	                Address:   0x01  (OUT)
	                Attributes:   0x02  (Bulk)
	                Max Packet Size:   64
	                Polling Interval:   0 ms
	        Interface #1 - Application Specific/Device Firmware Update "Fadecandy Bootloader"
	            Alternate Setting   0
	            Number of Endpoints   0
	            Interface Class:   254   (Application Specific)
	            Interface Subclass;   1   (Device Firmware Update)
	            Interface Protocol:   1
	            DFU Functional Descriptor   
	                bmAttributes:   0x0d (Download, No Upload, Manifestation Tolerant, Reserved bits: 0x08)
	                wDetachTimeout:   10000 ms
	                wTransferSize:   1024 bytes
