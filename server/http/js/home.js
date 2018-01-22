/*
 * Fadecandy Server Web UI
 *
 * This code is released into the public domain. Feel free to use it as a starting
 * point for your own apps that communicate with the Fadecandy Server.
 */

jQuery(function ($) {
    'use strict';

    var Utils = {

        escape: function(str) {
            return str.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
        },

        sensibleJsonToHtml: function(obj) {
            /*
             * Utility to make sensibly pretty-printed and hilighted JSON.
             * Small values are kept inline, larger arrays and objects are broken out onto multiple lines.
             */

            var visit = function(obj, indentLevel) {
                var objString = JSON.stringify(obj);
                var oneLiner = objString.length < 40;
                var items = [];
                var indent = '    ';
                var separator = oneLiner ? ' ' : '\n';
                indentLevel = oneLiner ? '' : (indentLevel || '');
                var nextIndent = oneLiner ? '' : (indentLevel + indent);

                // Simple value?
                if (obj == null || obj == undefined || typeof(obj) != "object") {
                    return '<span class="json-' + typeof(obj) + '">' + Utils.escape(objString) + '</span>';
                }

                if ($.isArray(obj)) {
                    // Array object
                    for (var i = 0; i < obj.length; i++) {
                        items.push(nextIndent + visit(obj[i], nextIndent));
                    }
                    return '<span class="json-punctuation">[</span>'
                        + separator + items.join(',' + separator)
                        + separator + indentLevel + '<span class="json-punctuation">]</span>';

                } else {
                    // Dictionary object
                    for (var k in obj) {
                        items.push(
                            nextIndent + '<span class="json-key">' + Utils.escape(JSON.stringify(k)) + '</span>'
                            + '<span class="json-punctuation">:</span> '
                            + visit(obj[k], nextIndent)
                        );
                    }
                    return '<span class="json-punctuation">{</span>'
                        + separator + items.join(',' + separator)
                        + separator + indentLevel + '<span class="json-punctuation">}</span>';
                }
            }

            return visit(obj);
        }
    };

    var Device = function(json) {
        this.json = json;
        this.onRemove = $.Callbacks();
        this.maxPixels = 512;
        this.alive = true;

        // Empty device view, just an item and heading
        this.view = $($.parseHTML('\
            <li class="list-group-item"> \
                <h4 class="list-group-item-heading"></h4> \
            </li> \
        '));

        // Other initialization is device-type-specific
        if (json.type == "fadecandy") {
            this.initTypeFadecandy();
        } else if (json.type == "apa102spi") {
            this.initTypeAPA102SPI();
        } else {
            this.initTypeOther();
        }

        // Append and slide into view
        $("#devices-list").append(this.view);
        this.view.hide();
        this.view.slideDown(400);
    }
    Device.prototype = {

        remove: function() {
            // Anyone with a dangling reference to this device should know it's dead
            this.alive = false;

            // Delete anyone else who's depending on this device
            this.onRemove.fire();

            // Destroy tooltips
            this.view.find("button").tooltip("destroy");

            // Slide out and delete from DOM
            this.view.slideUp(400, function() { $(this).remove() });
        },

        isEqual: function(json) {
            return this.json.type == json.type &&
                   this.json.serial == json.serial &&
                   this.json.timestamp == json.timestamp;
        },

        sendJson: function(message) {
            /*
             * Send a device-specific JSON message to this device.
             * The message is modified to include a "device" pattern which matches only
             * this device.
             */

            message.device = this.json;
            ConnectionManager.sendJson(message);
        },

        setStatusLed: function(state) {
            /*
             * Set the device's status LED state. True/false turns it on or off,
             * null sets it to the default behavior of blinking when a frame arrives.
             */
            this.sendJson({ 'type': 'device_options', 'options': { 'led': state }});
        },

        writePixels: function(pixels) {
            /*
             * Send pixel data directly to one device, bypassing the OPC mapping.
             * Pixels are an array of integers, three per pixel in RGB order.
             */
            this.sendJson({ 'type': 'device_pixels', 'pixels': pixels });
        },

        singleColorFrame: function(r, g, b) {
            /*
             * Create a full frame by repeating a single color
             */
            var a = new Array(this.maxPixels * 3);
            for (var i = 0; i < this.maxPixels; i++) {
                a[i*3] = r;
                a[i*3 + 1] = g;
                a[i*3 + 2] = b;
            }
            return a;
        },

        fadeInSingleFrame: function(pixels) {
            /*
             * The Fadecandy device interpolates between frames by default. If we
             * send it a single frame, it could take arbitrarily long to fade to that
             * frame, since the device may have an arbitrarily low expected frame
             * rate due to the time elapsed since the last frame it received.
             *
             * To make sure the frame happens in a well-defined amount of time, one
             * easy and good-looking solution is to send a black frame some time
             * before the intended frame so we can control the interpolation time.
             * If we send two black frames, we'll be sure that the lights go dark
             * immediately and we can control how long it takes for them to fade back up.
             */

            var device = this;
            var blackness = this.singleColorFrame(0,0,0);

            this.writePixels(blackness);
            this.writePixels(blackness);

            window.setTimeout(function() {
                if (device.alive) {
                    device.writePixels(pixels);
                }
            }, 300);
        },

        modalAction: function(title, body) {
            /*
             * Orchestrate a modal action that begins because of user interaction,
             * and ends when explicitly cancelled or when the device is removed.
             *
             * Returns the modal jQuery object.
             */

            var m = $($.parseHTML(' \
                <div class="modal"> \
                  <div class="modal-dialog"> \
                    <div class="modal-content"> \
                      <div class="modal-header"> \
                        <h4 class="modal-title">' + title + '</h4> \
                      </div> \
                      <div class="modal-body">' + body + '</div> \
                      <div class="modal-footer"> \
                        <button type="button" class="btn btn-primary">Enough</button> \
                      </div> \
                    </div> \
                  </div> \
                </div> \
            '));

            // Use onRemove callbacks to destroy if the device is removed
            var device = this;
            var removeCallback = function(){$(".modal").modal("hide"); }
            device.onRemove.add(removeCallback);

            m.on("hidden.bs.modal", function(){
                // Destructor
                device.onRemove.remove(removeCallback);
                $(".modal, .modal-backdrop").remove();
            });

            m.appendTo("body");
            m.modal("show");
            m.on("click", "button", null, function(){ $(".modal").modal("hide") });

            // Common text fields
            m.find(".device-serial").text(device.json.serial);
            m.find(".device-port").text(device.json.port);
            m.find(".device-numLights").text(device.json.numLights);

            return m;
        },

        initTypeFadecandy: function() {
            /*
             * For Fadecandy devices, we can show some meaningful properties, and we can
             * show a dropdown with actions to perform on those devices.
             */

            var device = this;

            this.view.find(".list-group-item-heading")
                .text("Fadecandy LED Controller")
                .after('\
                    <p> \
                        Serial number <code class="device-serial"></code>, \
                        firmware version <code class="device-version"></code> \
                    </p> \
                    <div class="btn-group"> \
                        \
                        <button type="button" class="btn btn-default action-identify">Identify</button> \
                        \
                        <div class="btn-group"> \
                            <button type="button" class="btn btn-default dropdown-toggle" data-toggle="dropdown"> \
                                Test Patterns <span class="caret"></span> \
                            </button> \
                            <ul class="dropdown-menu" role="menu"> \
                                <li><a class="action-off" href="#">All pixels off</a></li> \
                                <li><a class="action-full" href="#">Full brightness</a></li> \
                                <li><a class="action-half" href="#">50% brightness</a></li> \
                            </ul> \
                        </div> \
                    </div> \
                ');

            this.view.find(".device-serial").text(this.json.serial);
            this.view.find(".device-version").text(this.json.version);

            this.view.find(".action-identify")
                .tooltip({
                    placement: "bottom",
                    title: "Identify this device by flashing its built-in status LED",
                    container: "body",
                })
                .click(function (evt) {
                    var m = device.modalAction(
                        "Identifying Fadecandy Device", ' \
                        <p> \
                            The device with serial <code class="device-serial"></code> is \
                            flashing its built-in status LED. \
                        </p> \
                        <h1 class="text-center"> \
                            <span class="blinker">☀</span> \
                        </h1> \
                    ');

                    // Animate until the dialog is closed, then go back to the default LED options
                    var running = true;                          
                    m.on("hide.bs.modal", function(){
                        running = false;
                        device.setStatusLed(null);
                    });

                    var blinker = m.find(".blinker");
                    var blinkOff = function() {
                        if (running) {
                            device.setStatusLed(false);
                            blinker.delay(200).fadeTo(50, 1, function () { blinkOn() });
                        }
                    };
                    var blinkOn = function() {
                        if (running) {
                            device.setStatusLed(true);
                            blinker.delay(200).fadeTo(50, 0, function () { blinkOff() });
                        }
                    };
                    blinkOff();
                });

            this.view.find(".action-off").click(function (evt) {
                device.fadeInSingleFrame( device.singleColorFrame(0, 0, 0) );
            });

            this.view.find(".action-half").click(function (evt) {
                device.fadeInSingleFrame( device.singleColorFrame(128, 128, 128) );
            });

            this.view.find(".action-full").click(function (evt) {
                device.fadeInSingleFrame( device.singleColorFrame(255, 255, 255) );
            });
        },

        initTypeAPA102SPI: function () {
            /*
             * For Fadecandy devices, we can show some meaningful properties, and we can
             * show a dropdown with actions to perform on those devices.
             */

            var device = this;

            this.view.find(".list-group-item-heading")
                .text("APA102/APA102C/SK9822 Device via SPI")
                .after('\
                    <p> \
                        SPI Port <code class="device-port"></code>, \
                        Number of Lights <code class="device-numLights"></code> \
                    </p> \
                    <div class="btn-group"> \
                        \
                        <div class="btn-group"> \
                            <button type="button" class="btn btn-default dropdown-toggle" data-toggle="dropdown"> \
                                Test Patterns <span class="caret"></span> \
                            </button> \
                            <ul class="dropdown-menu" role="menu"> \
                                <li><a class="action-off" href="#">All pixels off</a></li> \
                                <li><a class="action-full" href="#">Full brightness</a></li> \
                                <li><a class="action-half" href="#">50% brightness</a></li> \
                                <li><a class="action-red-full" href="#">Full Red Only</a></li> \
                                <li><a class="action-green-full" href="#">Full Green Only</a></li> \
                                <li><a class="action-blue-full" href="#">Full Blue Only</a></li> \
                            </ul> \
                        </div> \
                    </div> \
                ');

            this.view.find(".device-port").text(this.json.port);
            this.view.find(".device-numLights").text(this.json.numLights);
            this.maxPixels = this.json.numLights;

            this.view.find(".action-off").click(function (evt) {
                device.fadeInSingleFrame(device.singleColorFrame(0, 0, 0));
            });

            this.view.find(".action-half").click(function (evt) {
                device.fadeInSingleFrame(device.singleColorFrame(128, 128, 128));
            });

            this.view.find(".action-full").click(function (evt) {
                device.fadeInSingleFrame(device.singleColorFrame(255, 255, 255));
            });

            this.view.find(".action-red-full").click(function (evt) {
                device.fadeInSingleFrame(device.singleColorFrame(255, 0, 0));
            });

            this.view.find(".action-green-full").click(function (evt) {
                device.fadeInSingleFrame(device.singleColorFrame(0, 255, 0));
            });

            this.view.find(".action-blue-full").click(function (evt) {
                device.fadeInSingleFrame(device.singleColorFrame(0, 0, 255));
            });
        },

        initTypeOther: function() {
            /*
             * Some other kind of device that our web frontend doesn't support.
             * As of this writing, there's already one of these: the Enttec DMX Pro.
             *
             * We don't support any actions, just show raw JSON info about the device.
             *
             * Note that this doesn't rely on any device properties. You can test it
             * by modifying the constructor to use this initializer for other device types.
             */

            this.view.find(".list-group-item-heading")
                .text("Other Device")
                .after('<p>Properties:</p>', '<pre></pre>');

            this.view.find("pre").html(Utils.sensibleJsonToHtml(this.json));
        }
    };

    var ConnectionManager = {

        init: function() {
            // Compatibility check
            if (!this.isBrowserSupported())
                return;
            $("#browser-not-supported").addClass("hide");

            // Start with an empty devices list
            $("#devices-list").empty();
            this.devices = [];

            // Make the initial connection attempt. We can reconnect manually if the connection is lost
            $(".connect-button").on("click", function(evt) {
                ConnectionManager.connect()
            });
            this.connect();
        },

        isBrowserSupported: function() {
            // Currently we only care about WebSockets
            return window.WebSocket;
        },

        connect: function() {
            /*
             * (Re)connect to the server. This manages our WebSocket's life cycle, and 
             * updates the UI according to our current connection state.
             */

            this.serverURL = window.location.origin.replace("http://", "ws://");
            this.ws = new WebSocket(this.serverURL);
            this.ws.onerror = this.onError;
            this.ws.onclose = this.onClose;
            this.ws.onopen = this.onOpen;
            this.ws.onmessage = this.onMessage;

            $("#server-url").text(this.serverURL);
            $("#connection-error,#connection-closed,#connection-complete").addClass("hide");
            $("#connection-in-progress").removeClass("hide");
        },

        sendJson: function(message) {
            /*
             * Send a JSON message to fcserver. Note that it's also possible to send binary
             * messages with raw OPC packets, and this is the preferred way to do high-performance
             * animation.
             */

            this.ws.send(JSON.stringify(message));
        },

        onError: function() {
            $("#connection-error").removeClass("hide");
            $("#connection-complete,#connection-in-progress").addClass("hide");
        },

        onClose: function() {
            $("#connection-closed").removeClass("hide");
            $("#connection-complete,#connection-in-progress").addClass("hide");

            // Delete all devices, this resets their state
            for (var i = 0; i < ConnectionManager.devices.length; i++) {
                ConnectionManager.devices[i].remove();
            }
            ConnectionManager.devices = [];
        },

        onOpen: function() {
            $("#connection-complete").removeClass("hide");
            $("#connection-in-progress").addClass("hide");

            // Fire off some requests to ask about the current server state
            ConnectionManager.sendJson({ type: "server_info" });
            ConnectionManager.sendJson({ type: "list_connected_devices" });
        },

        onMessage: function(evt) {
            var msg = JSON.parse(evt.data);

            if (msg.type == "server_info") {
                $("#server-version").text(msg.version);
                $("#server-config").html(Utils.sensibleJsonToHtml(msg.config));
                return;
            }

            if (msg.type == "list_connected_devices" ||         // Initial connection list
                msg.type == "connected_devices_changed") {      // Hotplug event 
                ConnectionManager.updateDeviceList(msg.devices);
                return;
            }
        },

        updateDeviceList: function(devices) {
            /*
             * We received a new list of devices, either after our connection completed
             * or after a hotplug event. Walk the list of devices and update our model.
             */

            // Are there any devices at all?
            if (devices.length == 0) {
                $("#no-devices-connected").removeClass("hide");
                $("#devices-list").addClass("hide");
            } else {
                $("#no-devices-connected").addClass("hide");
                $("#devices-list").removeClass("hide");
            }

            // Did any devices go missing?
            for (var i = 0; i < this.devices.length;) {
                var found = false;
                for (var j = 0; j < devices.length; j++) {
                    if (this.devices[i].isEqual(devices[j])) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    i++;
                } else {
                    this.devices[i].remove();
                    this.devices.splice(i, 1);
                }
            }

            // Are there any new devices?
            for (var i = 0; i < devices.length; i++) {
                var found = false;
                for (var j = 0; j < this.devices.length; j++) {
                    if (this.devices[j].isEqual(devices[i])) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    this.devices.push(new Device(devices[i]));
                }
            }
        }
    }

    ConnectionManager.init();
});