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

        // Empty device view, just an item and heading
        this.view = $.parseHTML('\
            <li class="list-group-item"> \
                <h4 class="list-group-item-heading"></h4> \
            </li> \
        ');

        // Other initialization is device-type-specific
        if (json.type == "fadecandy") {
            this.initTypeFadecandy();
        } else {
            this.initTypeOther();
        }

        // Append and slide into view
        $("#devices-list").append(this.view);
        $(this.view).hide();
        $(this.view).slideDown(400);
    }
    Device.prototype = {

        remove: function() {
            // Slide out and delete from DOM
            $(this.view).slideUp(400, function() { this.remove() });
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

        initTypeFadecandy: function() {
            /*
             * For Fadecandy devices, we can show some meaningful properties, and we can
             * show a dropdown with actions to perform on those devices.
             */

            $(this.view).find(".list-group-item-heading")
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
                                <li><a href="#">Action</a></li> \
                                <li><a href="#">Another action</a></li> \
                                <li><a href="#">Something else here</a></li> \
                                <li class="divider"></li> \
                                <li><a href="#">Separated link</a></li> \
                            </ul> \
                        </div> \
                    </div> \
                ');

            $(this.view).find(".device-serial").text(this.json.serial);
            $(this.view).find(".device-version").text(this.json.version);

            $(this.view).find(".action-identify")
                .tooltip({
                    placement: "bottom",
                    title: "Identify this device by flashing its built-in status LED",
                    container: "body",
                })
                .click(
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

            $(this.view).find(".list-group-item-heading")
                .text("Other Device")
                .after('<p>Properties:</p>', '<pre></pre>');

            $(this.view).find("pre").html(Utils.sensibleJsonToHtml(this.json));
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