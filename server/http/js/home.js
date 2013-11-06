/*
 * Fadecandy Server web UI
 */

$(document).ready(function(){

    // Do we support the browser? Right now we only care about checking for WebSockets.
    if (window.WebSocket) {
        $("#browser-not-supported").addClass("hide");
    } else {
        return;
    }

    var ws;
    var serverURL = window.location.origin.replace("http://", "ws://");
    $("#server-url").text(serverURL);

    // (Re)connect to the server
    var connect = function() {
        $("#connection-error,#connection-closed,#connection-complete").addClass("hide");
        $("#connection-in-progress").removeClass("hide");

        ws = new WebSocket(serverURL);

        ws.onerror = function(evt) {
            $("#connection-error").removeClass("hide");
            $("#connection-complete,#connection-in-progress").addClass("hide");
        };

        ws.onclose = function(evt) {
            $("#connection-closed").removeClass("hide");
            $("#connection-complete,#connection-in-progress").addClass("hide");
        };

        ws.onopen = function(evt) {
            $("#connection-complete").removeClass("hide");
            $("#connection-in-progress").addClass("hide");

            // Fire off some requests to ask about the current server state
            ws.send(JSON.stringify({ type: "server_info" }));
            ws.send(JSON.stringify({ type: "list_connected_devices" }));
        };

        ws.onmessage = function(evt) {
            var msg = JSON.parse(evt.data);

            if (msg.type == "server_info") {
                $("#server-version").text(msg.version);
                $("#server-config").text(msg.config);
            }
        };
    };

    // We can reconnect manually if the connection is lost
    $(".connect-button").click(connect);

    // Make the initial connection attempt
    connect();
});