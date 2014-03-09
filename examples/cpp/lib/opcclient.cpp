/*
 * Simple and efficient C++ client for Open Pixel Control
 *
 * Copyright (c) 2014 Micah Elizabeth Scott <micah@scanlime.org>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include "opcclient.h"


OPCClient::OPCClient()
{
    fd = -1;
    memset(&address, 0, sizeof address);
}

OPCClient::~OPCClient()
{
    closeSocket();
}

void OPCClient::closeSocket()
{
    if (isConnected()) {
        close(fd);
        fd = -1;
    }
}

bool OPCClient::resolve(const char *hostport, int defaultPort)
{
    fd = -1;

    char *host = strdup(hostport);
    char *colon = strchr(host, ':');
    int port = defaultPort;
    bool success = false;

    if (colon) {
        *colon = '\0';
        port = strtol(colon + 1, 0, 10);
    }

    if (port) {
        struct addrinfo *addr;
        getaddrinfo(*host ? host : "localhost", 0, 0, &addr);

        for (struct addrinfo *i = addr; i; i = i->ai_next) {
            if (i->ai_family == PF_INET) {
                memcpy(&address, i->ai_addr, sizeof address);
                address.sin_port = htons(port);
                success = true;
                break;
            }
        }
        freeaddrinfo(addr);
    }

    free(host);
    return success;
}

bool OPCClient::isConnected()
{
    return fd > 0;
}

bool OPCClient::write(const uint8_t *data, ssize_t length)
{
    if (!isConnected()) {
        if (!connectSocket()) {
            return false;
        }
    }

    while (length > 0) {
        int result = send(fd, data, length, 0);
        if (result <= 0) {
            closeSocket();
            return false;
        }
        length -= result;
        data += result;
    }

    return true;
}

bool OPCClient::write(const std::vector<uint8_t> &data)
{
    return write(&data[0], data.size());
}

bool OPCClient::connectSocket()
{
    fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (connect(fd, (struct sockaddr*) &address, sizeof address) < 0) {
        close(fd);
        return false;
    }

    int flag = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*) &flag, sizeof flag);

    return true;
}
