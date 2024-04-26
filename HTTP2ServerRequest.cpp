
#include "HTTP2ServerRequest.h"

namespace DB {
    void HTTP2ServerRequest::addHeader(const u_int8_t* name, size_t namelen, const u_int8_t* value, size_t valuelen) {
        std::string name_str((char*)name, namelen);
        std::string value_str((char*)value, valuelen);
        headers.insert({name_str, value_str});
    }

    std::string HTTP2ServerRequest::getHeader(const std::string &name) {
        if (auto it = headers.find(name); it != headers.end()) {
            return it->second;
        }
        return "";
    }
} // DB