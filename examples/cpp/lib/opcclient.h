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

#pragma once
#include <stdint.h>
#include <netinet/in.h>
#include <vector>

class OPCClient {
public:
    OPCClient();
    ~OPCClient();

    bool resolve(const char *hostport, int defaultPort = 7890);
    bool write(const uint8_t *data, ssize_t length);
    bool write(const std::vector<uint8_t> &data);
    bool isConnected();

    struct Header {
        uint8_t channel;
        uint8_t command;
        uint8_t length[2];

        void init(uint8_t channel, uint8_t command, uint16_t length) {
            this->channel = channel;
            this->command = command;
            this->length[0] = length >> 8;
            this->length[1] = (uint8_t)length;
        }

        uint8_t *data() {
            return (uint8_t*) &this[1];
        }

        // Use a Header() to manipulate packet data in a std::vector
        static Header& view(std::vector<uint8_t> &data) {
            return *(Header*) &data[0];
        }
    };

    // Commands
    static const uint8_t SET_PIXEL_COLORS = 0;

private:
    int fd;
    struct sockaddr_in address;
    bool connectSocket();
    void closeSocket();
};
