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
#include <string.h>
#include <fcntl.h>
#include <iostream>
#include <algorithm>
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
# include <sys/select.h>
# include <unistd.h>
# include <signal.h>
# define SOCKOPT_ARG(x)     (x)
# define RECV_BUF(x)        (x)
#endif


OPCSink::OPCSink(callback_t cb, void *context, bool verbose)
    : mVerbose(verbose), mCallback(cb), mContext(context), mSocket(-1), mThread(0)
{}

void OPCSink::start(struct addrinfo *listenAddr)
{
    mSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (mSocket < 0) {
        perror("socket");
        return;
    }

    int arg = 1;
    setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, SOCKOPT_ARG(&arg), sizeof arg);
    setNonBlock(mSocket);

    if (bind(mSocket, listenAddr->ai_addr, listenAddr->ai_addrlen)) {
        perror("bind");
        return;
    }

    if (listen(mSocket, 4) < 0) {
        perror("listen");
        return;
    }

    if (mVerbose) {
        struct sockaddr_in *sin = (struct sockaddr_in*) listenAddr->ai_addr;
        std::clog << "Listening on " << inet_ntoa(sin->sin_addr) << ":" << ntohs(sin->sin_port) << "\n";
    }

    mThread = new tthread::thread(threadWrapper, this);
}

void OPCSink::setNonBlock(int fd)
{
    fcntl(fd, F_SETFL, O_NONBLOCK);
}

void OPCSink::threadWrapper(void *arg)
{
    OPCSink *self = (OPCSink*) arg;
    self->threadFunc();
}

void OPCSink::threadFunc()
{
    while (1) {
        fd_set rfds, efds;
        int nfds = mSocket + 1;

        FD_ZERO(&rfds);
        FD_ZERO(&efds);
        FD_SET(mSocket, &rfds);
        FD_SET(mSocket, &efds);
        for (std::list<Client>::iterator i = mClients.begin(); i != mClients.end(); i++) {
            FD_SET(i->socket, &rfds);
            FD_SET(i->socket, &efds);
            nfds = std::max(nfds, i->socket + 1);
        }

        int r = select(nfds, &rfds, 0, &efds, 0);
        if (r < 0) {
            perror("select");
            return;
        }

        // Any new clients to add?
        if (FD_ISSET(mSocket, &rfds) || FD_ISSET(mSocket, &efds)) {
            pollAccept();
        }

        // Poll all existing clients, removing any that are dead
        std::list<Client>::iterator current = mClients.begin();
        std::list<Client>::iterator end = mClients.end();
        while (current != end) {
            std::list<Client>::iterator next = current;
            next++;

            if (FD_ISSET(current->socket, &rfds) || FD_ISSET(current->socket, &efds)) {
                if (!pollClient(*current)) {
                    // Lost a client
                    
                    if (mVerbose) {
                        std::clog << "Client disconnected\n";
                    }

                    close(current->socket);                
                    mClients.erase(current);
                }
            }

            current = next;
        }
    }
}

void OPCSink::pollAccept()
{
    // Can we accept any new clients?

    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof clientAddr;

    int sock = accept(mSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (sock < 0) {
        if (errno != EWOULDBLOCK) {
            perror("accept");
        }
        return;
    }

    setNonBlock(mSocket);

    // Disable nagle algorithm, we want low-latency
    int arg = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, SOCKOPT_ARG(&arg), sizeof arg);

    // New client!
    mClients.emplace_back();
    Client &client = mClients.back();

    client.bufferPos = 0;
    client.socket = sock;

    if (mVerbose) {
        std::clog << "Client connected from " << inet_ntoa(clientAddr.sin_addr) << "\n";
    }
}

bool OPCSink::pollClient(Client &client)
{
    // Check an existing client for new data. Returns false if the connection is lost.

    int r = recv(client.socket, RECV_BUF(client.bufferPos + (uint8_t*)&client.buffer),
        sizeof(client.buffer) - client.bufferPos, 0);

    if (r < 0) {
        if (errno == EWOULDBLOCK) {
            return true;
        } else {
            perror("read error");
            return false;
        }
    }

    if (r == 0) {
        // Client disconnecting
        return false;
    }

    // Enqueue new packet
    client.bufferPos += r;

    // Process any and all complete packets from our buffer
    while (1) {
        if (client.bufferPos < offsetof(Message, data)) {
            // Still waiting for a header
            break;
        }

        unsigned length = offsetof(Message, data) + client.buffer.length();
        if (client.bufferPos < length) {
            // Waiting for more data
            break;
        }

        // Complete packet.
        mCallback(client.buffer, mContext);

        // Save any part of the following packet we happened to grab.
        memmove(&client.buffer, length + (uint8_t*)&client.buffer, client.bufferPos - length);
        client.bufferPos -= length;
    }

    return true;
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