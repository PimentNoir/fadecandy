Fadecandy: WebSocket Protocol
=============================

The Fadecandy Server (`fcserver`) operates as a bridge between LED controllers attached over USB, and visual effects that communicate via a TCP socket.

The primary protocol supported by fcserver is [Open Pixel Control](http://openpixelcontrol.org), a super simple way to send RGB values over a socket. But in addition to OPC, fcserver also supports [WebSockets](http://www.websocket.org/) to allow clients to be written in Javascript.

Connecting
----------

To connect to fcserver, create a WebSocket. By default, fcserver listens on port 7890.

You can try this out in the Javascript Console in your web browser:

```
> ws = new WebSocket("ws://localhost:7890")
WebSocket {binaryType: "blob", extensions: "", protocol: "", onclose: null, onerror: null…}
```

In a real application, you'll also want to handle events like onopen() and onclose(). Take a look at the full [WebSockets API](http://dev.w3.org/html5/websockets/) if you haven't seen it yet.

Open Pixel Control Packets
--------------------------

Any binary packets sent over this socket are treated as Open Pixel Control packets. Since the WebSocket protocol includes its own length field, the original two length bytes in the OPC packet are reserved and should be sent as zero.

Channel    | Command   | Reserved (Zero) | Data
---------- | --------- | --------------- | --------------------------
1 byte     | 1 byte    | 2 bytes         | N bytes of message data

To send a binary packet, you can use [Typed Arrays](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Typed_arrays). For example, this sets the first three LEDs to red, green, and blue respectively. (We send the packet twice to force interpolation to happen immediately, since we don't have a predictable frame rate when entering commands like this.)

```
> ws = new WebSocket("ws://localhost:7890")
> packet = new Uint8Array([ 0,0,0,0, 255,0,0, 0,255,0, 0,0,255 ]);
> ws.send(packet.buffer)
> ws.send(packet.buffer)
```

This is the recommended way of sending pixel data from a web app. The mapping settings from the fcserver config file take effect, and any OPC packet can be sent in this way.

JSON Packets
------------

Any WebSocket frames that contain text rather than binary data are expected to be JSON. These packets can be sent from client to server or from server to client. All JSON packets are objects with a "type" string.

Unless otherwise specified, all command packets that can be successfully parsed will generate replies by the server. If a problem occurred in processing the command, an "error" member will be added with an error message string to explain the problem. When the server replies to a command from the client, additional JSON object members in the command are preserved so clients can associate metadata with requests.

list_connected_devices
----------------------

This is sent from client to server to ask for a list of all connected devices:

```
{ "type": "list_connected_devices" }
```

The server will respond with a message like:

```
{
    "type": "list_connected_devices",
    "devices": [
        {
            "type": "fadecandy",
            "serial": "ENICCULVLDQJQDWD",
            "timestamp": 1384320928627,
            "version": "1.06",
            "bcd_version": 262
        }
    ]
}
```

Each device is a JSON object within a list of devices. Each device object must uniquely identify a particular connection of a particular device. Fields may include:

Name         | Description
------------ | -----------------------------------------------------------
type         | String identifying the device type ("fadecandy", "enttec")
serial       | Serial number for this particular device instance
timestamp    | When did this device connect? Timestamp in milliseconds
version      | Firmware version for the device, as a string
bcd_version  | BCD encoded firmware version, from the USB descriptors

connected_devices_changed
-------------------------

This packet can be sent unsolicited by the server any time a new device is attached or an existing device is removed. The response is identical to **list_connected_devices**, aside from the packet type.

server_info
-----------

This is sent from client to server to ask for information about the server itself:

```
{ "type": "server_info" }
```

The server will respond with a message like:

```
{
    "type": "server_info",
    "version": "fcserver-1.01",
    "config": {
        "listen": ["127.0.0.1",7890],
        "verbose": true,
        "color": {"gamma": 2.5, "whitepoint": [1,1,1]},
        "devices": [{"type": "fadecandy", "map": [[0,0,0,512]]}]
    }
}
```

Fields in the response include:

Name         | Description
------------ | --------------------------------------------------------------------
version      | Server version string
config       | JSON object with the server's current configuration file contents

device_color_correction
-----------------------

Write color correction info directly to a single device. Does not change the global color correction settings. The client first provides a message with color correction settings in the same format as the server's configuration file, and a device specification in the same format as **list_connected_devices**.

This example changes the whitepoint on a specific Fadecandy controller, to make the brightness very dim:

```
{
    "type": "device_color_correction",
    "device": {
        "type": "fadecandy",
        "serial": "ENICCULVLDQJQDWD"
    },
    "color": {
        "gamma": 2.5,
        "whitepoint": [0.2,0.2,0.2]
    }
}
```

device_options
--------------

Change the configuration options for a single device. This loads the same options supported by the server's JSON configuration file in the section for a single device.

For example, a Fadecandy device supports the following options:

Name         | Values               | Default | Description
------------ | -------------------- | ------- | --------------------------------------------
led          | true / false / null  | null    | Is the LED on, off, or under automatic control?
dither       | true / false         | true    | Is dithering enabled?
interpolate  | true / false         | true    | Is inter-frame interpolation enabled?

This example turns on the LED on a specific Fadecandy controller:

```
{
    "type": "device_options",
    "device": {
        "type": "fadecandy",
        "serial": "ENICCULVLDQJQDWD"
    },
    "options": {
        "led": true
    }
}
```

device_pixels
-------------

Sends pixel data directly to a single device, bypassing the OPC mapping. This accepts pixel data as an array of integers, and it's a lot less efficient than using OPC packets. This is mostly useful for special-purpose clients like configuration tools.

For example, setting the first three pixels to red, green, and blue for a specific Fadecandy controller:

```
{
    "type": "device_pixels",
    "device": {
        "type": "fadecandy",
        "serial": "ENICCULVLDQJQDWD"
    },
    "pixels": [
		255, 0, 0,
		0, 255, 0,
		0, 0, 255
    ]
}
```
