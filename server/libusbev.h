/*
 * Glue for using libusb with libev
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
#include <libusb.h>
#include <map>

class LibUSBEventBridge
{
public:

    // Call after libusb_init
    void init(struct libusb_context *ctx, struct ev_loop *loop);

private:
    struct Watcher {
        ev_io io;
        LibUSBEventBridge *self;
    };

    struct libusb_context *mCtx;
    struct ev_loop *mLoop;
    std::map<int, Watcher*> mWatchers;

    static void cbEvent(struct ev_loop *loop, ev_io *io, int revents);
    static void cbAdded(int fd, short events, void *user_data);
    static void cbRemoved(int fd, void *user_data);
};
