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
Configurations  | 1
Endpoints       | 1
Endpoint 1      | Bulk OUT (Host to Device), 64-byte packets

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
1           | 7 … 2  | (reserved)
1           | 3      | Manual LED control bit
1           | 2      | 0 = LED shows USB activity, 1 = LED under manual control
1           | 1      | Disable keyframe interpolation
1           | 0      | Disable dithering
2 … 63      | 7 … 0  | (reserved)

Control Requests
----------------

In addition to the OUT endpoint, the device also supports vendor-specific control requests:

bmRequestType | bRequest | wValue | wIndex | wLength | Description
------------- | -------- | ------ | ------ | ------- | ---------------------------------------------
0xC0          | 0x01     | 0      | 0      | 4       | Read rendered frame counter (32-bit, little endian)
0xC0          | 0x01     | 0      | 1      | 4       | Read received keyframe counter (32-bit, little endian)
0xC0          | 0x7E     | x      | 4      | x       | Read Microsoft WCID descriptor
0xC0          | 0x7E     | x      | 5      | x       | Read Microsoft Extended Properties descriptor

