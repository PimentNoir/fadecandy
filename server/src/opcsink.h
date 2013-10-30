/*
 * Open Pixel Control server for Fadecandy
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
#include <ev.h>
#include <stdint.h>

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
    void start(struct ev_loop *loop, struct addrinfo *listenAddr);

    // Portable socket utilities
    static bool socketInit();
    static struct addrinfo* newAddr(const char *host, int port);
    static void freeAddr(struct addrinfo* addr);

private:
    bool mVerbose;
    callback_t mCallback;
    void *mContext;
    struct ev_io mIOAccept;

    struct Client {
        struct ev_io ioRead;
        Message buffer;
        unsigned bufferPos;
        OPCSink *self;
    };

    static void cbAccept(struct ev_loop *loop, struct ev_io *watcher, int revents);
    static void cbRead(struct ev_loop *loop, struct ev_io *watcher, int revents);
};
