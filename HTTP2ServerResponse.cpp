#include "HTTP2ServerResponse.h"
#include "HTTP2ServerStream.h"
#include "HTTP2ServerConnection.h"

namespace DB {
    void HTTP2ServerResponse::send() {
        stream->connection->sendResponse(stream);
    }

    bool HTTP2ServerResponse::hasHeader(const std::string &name) {
        auto it = headers.find(name);
        return it != headers.end();
    }

    std::string HTTP2ServerResponse::getHeader(const std::string &name) {
        if (auto it = headers.find(name); it != headers.end()) {
            return it->second;
        }
        return "";
    }

    void HTTP2ServerResponse::addHeader(const std::string& name, const std::string& value) {
        headers.insert({name, value});
    }
} // DB