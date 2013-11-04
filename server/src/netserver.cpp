/*
 * Open Pixel Control and HTTP server for Fadecandy.
 *
 * This is a TCP server which accepts either Open Pixel Control, HTTP, or WebSockets
 * connections on a single port. We use a fork of libwebsockets which supports serving
 * external non-HTTP protocols via a low-level receive callback.
 * 
 * Copyright (c) 2013 Micah Elizabeth Scott
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "netserver.h"
#include "version.h"
#include "libwebsockets.h"
#include <iostream>
#include <algorithm>


NetServer::NetServer(OPC::callback_t messageCallback, void *context, bool verbose)
    : mMessageCallback(messageCallback), mUserContext(context), mThread(0), mVerbose(verbose)
{}

bool NetServer::start(const char *host, int port)
{
    const int llNormal = LLL_ERR | LLL_WARN;
    const int llVerbose = llNormal | LLL_NOTICE;

    static struct libwebsocket_protocols protocols[] = {
        // first protocol must always be HTTP handler

        {
            "http-only",        // Name
            httpCallback,       // Callback
            sizeof(Client),     // Protocol-specific data size
            0,                  // Max frame size / rx buffer
        },

        { NULL, NULL, 0, 0 }    // terminator
    };

    struct lws_context_creation_info info;
    memset(&info, 0, sizeof info);
    info.gid = -1;
    info.uid = -1;
    info.host = host;
    info.port = port;
    info.protocols = protocols;
    info.user = this;

    // Quieter during create_context, since it's kind of chatty.
    lws_set_log_level(llNormal, NULL);

    struct libwebsocket_context *context = libwebsocket_create_context(&info);
    if (!context) {
        lwsl_err("libwebsocket init failed\n");
        return false;
    }

    // Maybe set up a more verbose log level now.
    if (mVerbose) {
        lws_set_log_level(llVerbose, NULL);
    }

    lwsl_notice("Server listening on %s:%d\n", host ? host : "*", port);

    // Note that we pass ownership of all libwebsockets state to this new thread.
    // We shouldn't access it on the other threads afterwards.
    mThread = new tthread::thread(threadFunc, context);

    return true;
}

void NetServer::threadFunc(void *arg)
{
    struct libwebsocket_context *context = (libwebsocket_context*) arg;
    while (libwebsocket_service(context, 1000) >= 0);
    libwebsocket_context_destroy(context);
}

int NetServer::httpCallback(libwebsocket_context *context, libwebsocket *wsi,
    enum libwebsocket_callback_reasons reason, void *user, void *in, size_t len)
{
    /*
     * Handle HTTP and non-websocket traffic.
     *
     * For HTTP, this serves simple static HTML documents which are included at compile-time
     * from the 'http' directory. Our web UI interacts with the server via the public WebSockets API.
     */

    NetServer *self = (NetServer*) libwebsocket_context_user(context);
    Client *client = (Client*) user;

    switch (reason) {
        case LWS_CALLBACK_HTTP:
            return self->httpBegin(context, wsi, *client, (const char*) in);

        case LWS_CALLBACK_HTTP_FILE_COMPLETION:
            // Only serve one file per connect
            return -1;

        case LWS_CALLBACK_HTTP_WRITEABLE:
            return self->httpWrite(context, wsi, *client);

        case LWS_CALLBACK_SOCKET_READ:
            // Low-level socket read. We may trap these for OPC protocol handling.
            if (client->state != CLIENT_STATE_HTTP) {
                return self->opcRead(context, wsi, *client, (uint8_t*)in, len);
            }
            break;

        default:
            break;
    }

    return 0;
}

int NetServer::opcRead(libwebsocket_context *context, libwebsocket *wsi,
    Client &client, uint8_t *in, size_t len)
{
    /*
     * Open Pixel Control packet dispatch, and protocol detection.
     *
     * Store the new packet in our protocol buffer. This should be large enough to never overflow,
     * since our OPC buffer is large enough for two packets and the OPC max packet size is much larger
     * than the network receive buffer.
     *
     * If we have no buffered data yet, we can do this without copying.
     */

    uint8_t *buffer;
    unsigned bufferLength;    

    if (len + client.bufferLength > sizeof client.buffer) {
        return -1;
    }

    if (client.bufferLength) {
        memcpy(client.bufferLength + client.buffer, in, len);
        buffer = client.buffer;
        bufferLength = client.bufferLength + len;
    } else {
        buffer = in;
        bufferLength = len;
    }

    if (client.state == CLIENT_STATE_PROTOCOL_DETECT) {
        /*
         * It's a new connection, and we aren't sure yet whether it's native OPC
         * or HTTP / WebSockets. We examine the first four bytes received. If it's
         * "GET ", we assume this is HTTP. (Other HTTP methods are not needed).
         * If it's anything else, we interpret the connection as native OPC and these
         * are the first four bytes of the first OPC packet.
         */

        if (bufferLength < 4) {
            // Not enough data for protocol detect yet. Save this data for later.

            if (buffer != client.buffer) {
                memcpy(client.buffer, buffer, bufferLength);
                client.bufferLength = bufferLength;
            }

            // Do not pass this data on to libwebsocket yet
            return 1;
        }

        if (buffer[0] == 'G' && buffer[1] == 'E' && buffer[2] == 'T' && buffer[3] == ' ') {
            // Detected HTTP. Convert this to an HTTP client, and let libwebsockets handle
            // all data received so far.

            client.state = CLIENT_STATE_HTTP;
            if (libwebsocket_read(context, wsi, buffer, bufferLength) < 0) {
                return -1;
            }
            return 1;
        }

        // Not HTTP. Handle this as an OPC socket.
        client.state = CLIENT_STATE_OPEN_PIXEL_CONTROL;
        lwsl_notice("New Open Pixel Control connection\n");
    }

    // Process any and all complete packets from our buffer
    while (1) {

        if (bufferLength < OPC::HEADER_BYTES) {
            // Still waiting for a header
            break;
        }

        OPC::Message *msg = (OPC::Message*) buffer;
        unsigned msgLength = OPC::HEADER_BYTES + msg->length();

        if (bufferLength < msgLength) {
            // Waiting for more data
            break;
        }

        // Complete packet.
        mMessageCallback(*msg, mUserContext);

        buffer += msgLength;
        bufferLength -= msgLength;
    }

    // If we have any residual data, save it for later.
    if (bufferLength && buffer != client.buffer) {
        memmove(client.buffer, buffer, bufferLength);
    }
    client.bufferLength = bufferLength;

    // Don't pass data on to libwebsockets
    return 1;
}

bool NetServer::httpPathEqual(const char *a, const char *b)
{
    // HTTP path comparison. Stop at '?' or '#', to ignore query/fragment portions.
    for (;;) {
        char ca = *a;
        char cb = *b;
        if (ca == '?' || cb == '?' || ca == '#' || cb == '#')
            return true;
        if (ca != cb)
            return false;
        if (ca == '\0')
            return true;
        a++;
        b++;
    }
}

int NetServer::httpBegin(libwebsocket_context *context, libwebsocket *wsi,
    Client &client, const char *path)
{
    /*
     * We have a new plain HTTP request. Match it against our document list, and send
     * back headers for the response.
     */

    HTTPDocument *doc = httpDocumentList;

    // Look for this path in the document list. If it isn't found, we'll serve the 404 doc.
    while (doc->path && !httpPathEqual(doc->path, path))
        doc++;

    if (!doc->path) {
        lwsl_notice("HTTP document not found, \"%s\"\n", path);
    }

    int size = snprintf((char*) client.buffer, sizeof client.buffer,
        "HTTP/1.1 %d %s\r\n"
        "Server: %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %u\r\n"
        "Connection: close\r\n"
        "\r\n",
        doc->path ? 200 : 404,
        doc->path ? "OK" : "Not Found",
        kFCServerVersion,
        doc->contentType,
        (int) strlen(doc->body)
    );

    if (libwebsocket_write(wsi, client.buffer, size, LWS_WRITE_HTTP) < 0) {
        return -1;
    }

    // Write the body asynchronously
    client.httpBody = doc->body;
    libwebsocket_callback_on_writable(context, wsi);

    return 0;
}

int NetServer::httpWrite(libwebsocket_context *context, libwebsocket *wsi, Client &client)
{
    if (!client.httpBody) {
        return -1;
    }

    do {
        int len = (int) strlen(client.httpBody);
        if (len == 0) {
            // End of document
            return -1;
        }
        int m = libwebsocket_write(wsi, (unsigned char *) client.httpBody, len, LWS_WRITE_HTTP);
        if (m < 0) {
            return -1;
        }
        client.httpBody += m;
    } while (!lws_send_pipe_choked(wsi));

    libwebsocket_callback_on_writable(context, wsi);
    return 0;
}
