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

#include "libusbev.h"
#include <libusbi.h>
#include <stdlib.h>


void LibUSBEventBridge::cbEvent(struct ev_loop *loop, ev_io *io, int revents)
{
    Watcher *w = container_of(io, Watcher, io);
    LibUSBEventBridge *self = w->self;

    // Handle pending USB events without blocking
    struct timeval tvZero = { 0, 0 };
    libusb_handle_events_timeout(self->mCtx, &tvZero);
}

void LibUSBEventBridge::cbAdded(int fd, short events, void *user_data)
{
    LibUSBEventBridge *self = static_cast<LibUSBEventBridge*>(user_data);
    Watcher *w = new Watcher();

    w->self = self;
    ev_io_init(&w->io, cbEvent, fd,
        ((events & POLLIN) ? EV_READ : 0) |
        ((events & POLLOUT) ? EV_WRITE : 0));

    self->mWatchers[fd] = w;
    ev_io_start(self->mLoop, &w->io);
}

void LibUSBEventBridge::cbRemoved(int fd, void *user_data)
{
    LibUSBEventBridge *self = static_cast<LibUSBEventBridge*>(user_data);
    Watcher *w = self->mWatchers[fd];

    ev_io_stop(self->mLoop, &w->io);
    delete w;
    self->mWatchers.erase(fd);
}

void LibUSBEventBridge::init(struct libusb_context *ctx, struct ev_loop *loop)
{
    mCtx = ctx;
    mLoop = loop;

    // Handle FDs that are already registered
    const struct libusb_pollfd **fds = libusb_get_pollfds(ctx);
    if (fds) {
        for (const struct libusb_pollfd **i = fds; *i; ++i) {
            cbAdded((*i)->fd, (*i)->events, this);
        }
        free(fds);
    }

    // Get notified when future callbacks are added
    libusb_set_pollfd_notifiers(ctx, cbAdded, cbRemoved, this);
}
