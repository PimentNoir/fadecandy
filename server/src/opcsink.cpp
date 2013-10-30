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
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sstream>

#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <winsock2.h>
# include <windows.h>
# include <ws2tcpip.h>
# define SOCKOPT_ARG(x)     ((const char*)(x))
# define RECV_BUF(x)        ((char*)(x))
#else
# include <netinet/in.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netdb.h>
# include <arpa/inet.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <unistd.h>
# include <signal.h>
# define SOCKOPT_ARG(x)     (x)
# define RECV_BUF(x)        (x)
#endif

#include <libusbi.h>


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
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, SOCKOPT_ARG(&arg), sizeof arg);

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
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, SOCKOPT_ARG(&arg), sizeof arg);

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

    int r = recv(watcher->fd, RECV_BUF(cli->bufferPos + (uint8_t*)&cli->buffer),
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

    // Enqueue new packet
    cli->bufferPos += r;

    // Process any and all complete packets from our buffer
    while (1) {
        if (cli->bufferPos < offsetof(Message, data)) {
            // Still waiting for a header
            return;
        }

        unsigned length = offsetof(Message, data) + cli->buffer.length();
        if (cli->bufferPos < length) {
            // Waiting for more data
            return;
        }

        // Complete packet.
        self->mCallback(cli->buffer, self->mContext);

        // Save any part of the following packet we happened to grab.
        memmove(&cli->buffer, length + (uint8_t*)&cli->buffer, cli->bufferPos - length);
        cli->bufferPos -= length;
    }
}

struct addrinfo* OPCSink::newAddr(const char *host, int port)
{
    std::ostringstream portStr;
    portStr << port;

    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = PF_UNSPEC;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *ai = 0;
    int error = getaddrinfo(host, portStr.str().c_str(), &hints, &ai);

    if (error) {
        std::clog << "getaddrinfo: " << gai_strerror(error) << "\n";
        ai = 0;
    }

    return ai;
}

void OPCSink::freeAddr(struct addrinfo* addr)
{
    freeaddrinfo(addr);
}

bool OPCSink::socketInit()
{
#ifdef _WIN32
    WSADATA wsaData;
    int error;

    error = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (error) {
        std::clog << "WSAStartup failed (Error " << error << ")\n";
        return false;
    }
#else
    signal(SIGPIPE, SIG_IGN);    
#endif

    return true;
}