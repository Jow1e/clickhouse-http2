#pragma once

#include <map>
#include <string>
#include <ostream>

#include <Poco/FIFOBuffer.h>

namespace DB {

    class HTTP2ServerConnection;

    class HTTP2ServerStream;

    class HTTP2ServerResponse {
    public:
        explicit HTTP2ServerResponse(HTTP2ServerStream* stream) : stream(stream), out(0) {}

        void send();

        bool hasHeader(const std::string& name);

        std::string getHeader(const std::string& name);

        void addHeader(const std::string& name, const std::string& value);

        HTTP2ServerStream* stream;
        std::map<std::string, std::string> headers;
        Poco::BasicFIFOBuffer<uint8_t> out;
    };

} // DB
