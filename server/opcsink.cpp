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

#include "opcsink.h"
#include "util.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <iostream>


OPCSink::OPCSink(callback_t cb, void *context, bool verbose)
    : mVerbose(verbose), mCallback(cb), mContext(context) {}

void OPCSink::start(struct ev_loop *loop, struct addrinfo *listenAddr)
{
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return;
    }

    int arg = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof arg);

    if (bind(sock, listenAddr->ai_addr, listenAddr->ai_addrlen)) {
        perror("bind");
        return;
    }

    if (listen(sock, 4) < 0) {
        perror("listen");
        return;
    }

    // Get a callback when we're ready to accept a new connection
    ev_io_init(&mIOAccept, cbAccept, sock, EV_READ);
    ev_io_start(loop, &mIOAccept);

    if (mVerbose) {
        struct sockaddr_in *sin = (struct sockaddr_in*) listenAddr->ai_addr;
        std::clog << "Listening on " << inet_ntoa(sin->sin_addr) << ":" << ntohs(sin->sin_port) << "\n";
    }
}

void OPCSink::cbAccept(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    OPCSink *self = container_of(watcher, OPCSink, mIOAccept);
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof clientAddr;

    int sock = accept(watcher->fd, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (sock < 0) {
        perror("accept");
        return;
    }

    int arg = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &arg, sizeof arg);

    Client *cli = new Client();
    cli->bufferPos = 0;
    cli->self = self;

    ev_io_init(&cli->ioRead, cbRead, sock, EV_READ);
    ev_io_start(loop, &cli->ioRead);

    if (self->mVerbose) {
        std::clog << "Client connected from " << inet_ntoa(clientAddr.sin_addr) << "\n";
    }
}

void OPCSink::cbRead(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    Client *cli = container_of(watcher, Client, ioRead);
    OPCSink *self = cli->self;

    int r = recv(watcher->fd, cli->bufferPos + (uint8_t*)&cli->buffer,
        sizeof(cli->buffer) - cli->bufferPos, 0);

    if (r < 0) {
        perror("read error");
        return;
    }

    if (r == 0) {
        // Client disconnecting

        if (self->mVerbose) {
            std::clog << "Client disconnected\n";
        }

        ev_io_stop(loop, watcher);
        delete cli;
        return;
    }

    cli->bufferPos += r;
    if (cli->bufferPos >= offsetof(Message, data)) {
        // We have a header, at least.

        unsigned length = offsetof(Message, data) + cli->buffer.length();
        if (cli->bufferPos >= length) {
            // Complete packet.
            self->mCallback(cli->buffer, self->mContext);

            // Save any part of the following packet we happened to grab.
            memmove(&cli->buffer, length + (uint8_t*)&cli->buffer, cli->bufferPos - length);
            cli->bufferPos -= length;
        }
    }
}
