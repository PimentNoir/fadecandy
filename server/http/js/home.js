/*
 * Fadecandy Server web UI
 */


/*
 * Utility to make sensibly pretty-printed and hilighted JSON.
 * Small values are kept inline, larger arrays and objects are broken out onto multiple lines.
 */

function SensibleJsonToHtml(obj)
{
    var escape = function(str) {
        return str.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
    }

    var visit = function(obj, indentLevel)
    {
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


$(document).ready(function()
{
    // Do we support the browser? Right now we only care about checking for WebSockets.
    if (window.WebSocket) {
        $("#browser-not-supported").addClass("hide");
    } else {
        return;
    }

    var ws;
    var serverURL = window.location.origin.replace("http://", "ws://");
    $("#server-url").text(serverURL);

    /*
     * (Re)connect to the server. This creates our WebSocket, and holds its callbacks.
     */

    function Connect()
    {
        $("#connection-error,#connection-closed,#connection-complete").addClass("hide");
        $("#connection-in-progress").removeClass("hide");

        ws = new WebSocket(serverURL);

        ws.onerror = function(evt)
        {
            $("#connection-error").removeClass("hide");
            $("#connection-complete,#connection-in-progress").addClass("hide");
        };

        ws.onclose = function(evt)
        {
            $("#connection-closed").removeClass("hide");
            $("#connection-complete,#connection-in-progress").addClass("hide");
        };

        ws.onopen = function(evt)
        {
            $("#connection-complete").removeClass("hide");
            $("#connection-in-progress").addClass("hide");

            // Fire off some requests to ask about the current server state
            ws.send(JSON.stringify({ type: "server_info" }));
            ws.send(JSON.stringify({ type: "list_connected_devices" }));
        };

        ws.onmessage = function(evt)
        {
            var msg = JSON.parse(evt.data);

            console.log(msg);

            if (msg.type == "server_info") {
                $("#server-version").text(msg.version);
                $("#server-config").html(SensibleJsonToHtml(msg.config));
            }
        };
    }

    // Make the initial connection attempt. We can reconnect manually if the connection is lost
    $(".connect-button").click(Connect);
    Connect();
});