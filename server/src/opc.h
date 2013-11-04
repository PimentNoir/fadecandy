/*
 * Open Pixel Control and HTTP server for Fadecandy
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

#pragma once
#include <stdint.h>
#include <list>
#include "tinythread.h"
#include "libwebsockets.h"

struct addrinfo;

class OPCSink {
public:
    enum Command {
        SetPixelColors = 0x00,
        SystemExclusive = 0xFF,
    };

    // SysEx system and command IDs
    enum SysEx {
        FCSetGlobalColorCorrection = 0x00010001,
        FCSetFirmwareConfiguration = 0x00010002
    };

    struct Message
    {
        uint8_t channel;
        uint8_t command;
        uint8_t lenHigh;
        uint8_t lenLow;
        uint8_t data[0xFFFF];

        unsigned length() const {
            return lenLow | (unsigned(lenHigh) << 8);
        }
    };

    typedef void (*callback_t)(Message &msg, void *context);

    OPCSink(callback_t cb, void *context, bool verbose = false);

    // Start the event loop on a separate thread
    void start(const char *host, int port);

private:
    enum ClientState {
        CLIENT_STATE_PROTOCOL_DETECT = 0,
        CLIENT_STATE_OPEN_PIXEL_CONTROL,
        CLIENT_STATE_HTTP
    };

    // In-memory database of static files to serve over HTTP
    struct HTTPDocument {
        const char *path;
        const char *body;
        const char *contentType;
    };

    struct Client {
        // Overall socket state
        int socket;
        ClientState state;
        struct libwebsocket *wsi;

        // HTTP response state
        const char *httpBody;

        // Low-level receive buffer
        unsigned bufferPos;
        uint8_t buffer[2 * sizeof(struct Message)];
    };

    bool mVerbose;
    callback_t mCallback;
    void *mContext;
    int mSocket;
    tthread::thread *mThread;
    std::list<Client> mClients;
    struct libwebsocket_context *mLWS;

    static HTTPDocument httpDocumentList[];

    bool startListening(struct addrinfo *listenAddr);
    bool startWebSockets();

    static void threadWrapper(void *arg);
    void threadFunc();
    void setNonBlock(int fd);

    void pollAccept();
    bool pollClient(Client &client);
    int handleBufferedPacket(Client &client);

    // libwebsockets callbacks
    static int callback_http(struct libwebsocket_context *context, struct libwebsocket *wsi,
        enum libwebsocket_callback_reasons reason, void *user, void *in, size_t len);
};
