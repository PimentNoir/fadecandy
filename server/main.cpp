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

#include "rapidjson/document.h"
#include "rapidjson/reader.h"
#include "rapidjson/filestream.h"
#include "fcserver.h"
#include <unistd.h>
#include <signal.h>
#include <cstdio>

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr,
            "\n"
            "Fadecandy Open Pixel Control server\n"
            "\n"
            "usage: fcserver <config.json>\n"
            "\n"
            "Copyright (c) 2013 Micah Elizabeth Scott <micah@scanlime.org>\n"
            "https://github.com/scanlime/fadecandy\n"
            "\n");
        return 1;
    }

    FILE *configFile = fopen(argv[1], "r");
    if (!configFile) {
        perror("Error opening config file");
        return 2;
    }

    rapidjson::FileStream istr(configFile);
    rapidjson::Document config;
    config.ParseStream<0>(istr);
    if (config.HasParseError()) {
        fprintf(stderr, "Parse error at character %ld: %s\n",
            config.GetErrorOffset(), config.GetParseError());
        return 3;
    }

    FCServer server(config);
    if (server.hasError()) {
        fprintf(stderr, "Configuration errors:\n%s", server.errorText());
        return 5;
    }

    struct ev_loop *loop = EV_DEFAULT;
    server.start(loop);
    signal(SIGPIPE, SIG_IGN);
    ev_run(loop, 0);

    return 0;
}
