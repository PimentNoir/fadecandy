Open Pixel Control Server
=========================

The Fadecandy OPC Server is a daemon which glues one or more [Open Pixel Control](http://openpixelcontrol.org) clients to one or more supported hardware devices.

Clients include:

* Your own light effects code, written in any language
* The Fadecandy color balance tool

Supported hardware devices:

* Any number of Fadecandy boards, hot-pluggable via USB.
* To do: [Enttec DMX USB Pro](http://www.enttec.com/?main_menu=Products&pn=70304) or compatible.

OPC Commands
------------

All OPC commands follow the same general format. All multi-byte values in Open Pixel Control are in network byte order, high byte followed by low byte.

Channel    | Command   | Length (N) | Data
---------- | --------- | ---------- | --------------------------
1 byte     | 1 byte    | 2 bytes    | N bytes of message data

Video data arrives in a **Set Pixel Colors** command:

Byte   | **Set Pixel Colors** command
------ | --------------------------------
0      | Channel Number
1      | Command (0x01)
2 - 3  | Data length
4      | Pixel #0, Red
5      | Pixel #0, Green
6      | Pixel #0, Blue
7      | Pixel #1, Red
8      | Pixel #1, Green
9      | Pixel #1, Blue
…      | …

As soon as a complete Set Pixel Colors command is received, a new frame of video will be broadcast simultaneously to all attached Fadecandy devices.

The color correction data (from the 'color' configuration key) can also be changed at runtime, by sending a new blob of JSON text in a Fadecandy-specific command. Fadecandy's 16-bit System ID for Open Pixel Control's System Exclusive (0xFF) command is **0x0001**.

Byte   | **Set Global Color Correction** command
------ | ------------------------------------------
0      | Channel Number (0x00, reserved)
1      | Command (0xFF, System Exclusive)
2 - 3  | Data length (JSON Length + 4)
4 - 5  | System ID (0x0001, Fadecandy)
6 - 7  | SysEx ID (0x0001, Set Global Color Correction)
8 - …  | JSON Text


Configuration
-------------

The JSON configuration file is a dictionary which contains global configuration and an array of device objects. For each device, a dictionary includes device properties as well as a mapping table with commands which wire outputs to their corresponding OPC inputs. The map is a list of objects which act as mapping commands. Supported mapping objects:

* [ *OPC Channel*, *First OPC Pixel*, *First output pixel*, *pixel count* ]
	* Map a contiguous range of pixels from the specified OPC channel to the current device
	* For Fadecandy devices, output pixels are numbered from 0 through 511. Strand 1 begins at index 0, strand 2 begins at index 64, etc.

The following example config file supports two Fadecandy devices with distinct serial numbers. They both receive data from OPC channel #0. The first 512 pixels map to the first Fadecandy device. The next 64 pixels map to the entire first strand of the second Fadecandy device, and the next 32 pixels map to the beginning of the third strand.

	{
		"listen": ["127.0.0.1", 7890],
		"verbose": true,

		"color": {
			"gamma": 2.5,
			"whitepoint": [0.98, 1.0, 1.0]
		},

		"devices": [
			{
				"type": "fadecandy",
				"serial": "FFFFFFFFFFFF00180017200214134D44",
				"map": [
					[ 0, 0, 0, 512 ]
				]
			},
			{
				"type": "fadecandy",
				"serial": "FFFFFFFFFFFF0021003B200314134D44",
				"map": [
					[ 0, 512, 0, 64 ],
					[ 0, 576, 128, 32 ]
				]
			}
		]
	}

Prerequisites
-------------

The OPC server has been designed to work smoothly on Mac OS and Linux. With some work, it's possible to get it running on Windows.

* Requires [libusbx](https://github.com/libusbx/libusbx) version 1.0.16 or later.
* Requires [libev](http://software.schmorp.de/pkg/libev.html).

On Mac OS, the required packages are easy to install with [Homebrew](http://brew.sh/):

	$ brew install libusbx libev

On Debian or Ubuntu Linux (including the Raspberry Pi) libev can be installed with apt-get, but currently you must compile libusbx yourself:

	$ sudo apt-get install libev-dev libtool autoconf automake libudev-dev
	$ git clone https://github.com/libusbx/libusbx.git
	$ cd libusbx
	$ ./autogen.sh
	$ ./configure
	$ make
	$ sudo make install
