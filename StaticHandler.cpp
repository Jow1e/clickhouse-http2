#include "HTTP2StaticHandler.h"

namespace DB {
    void HTTP2StaticHandler::handleRequest(HTTP2ServerRequest & request, HTTP2ServerResponse & response) {
        response.addHeader(":status", "200");
        unsigned char data[4] = "OK\n";
        response.out.write((uint8_t*)data, 2);
        response.send();
    }
} // DB