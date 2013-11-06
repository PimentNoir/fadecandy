/*
 * Fadecandy Server web UI
 */

jQuery(function ($) {
    'use strict';

    var Utils = {

        sensibleJsonToHtml: function(obj) {
            /*
             * Utility to make sensibly pretty-printed and hilighted JSON.
             * Small values are kept inline, larger arrays and objects are broken out onto multiple lines.
             */

            var escape = function(str) {
                return str.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
            }

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
                    return '<span class="json-' + typeof(obj) + '">' + escape(objString) + '</span>';
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
                            nextIndent + '<span class="json-key">' + escape(JSON.stringify(k)) + '</span>'
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

    var ConnectionManager = {

        init: function() {
            // Compatibility check
            if (!this.isBrowserSupported())
                return;
            $("#browser-not-supported").addClass("hide");

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
                $("#devices-table").addClass("hide");
            } else {
                $("#no-devices-connected").addClass("hide");
                $("#devices-table").removeClass("hide");
            }
        }
    }

    ConnectionManager.init();
});