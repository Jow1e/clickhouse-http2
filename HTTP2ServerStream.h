#pragma once

#include "HTTP2ServerRequest.h"
#include "HTTP2ServerResponse.h"

#include <memory>

namespace DB {

    class HTTP2ServerConnection;

    class HTTP2ServerStream {
    public:
        HTTP2ServerStream(int stream_id, HTTP2ServerConnection* connection)
        : stream_id(stream_id)
        , connection(connection)
        , response(this)
        {}

        HTTP2ServerRequest request;
        HTTP2ServerResponse response;
        HTTP2ServerConnection* connection;
        int stream_id;
    };

    using HTTP2ServerStreamPtr = std::unique_ptr<HTTP2ServerStream>;

} // DB

