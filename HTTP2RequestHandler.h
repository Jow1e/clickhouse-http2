#pragma once

#include "HTTP2ServerRequest.h"
#include "HTTP2ServerResponse.h"

namespace DB
{

    class HTTP2RequestHandler
    {
    public:
        virtual ~HTTP2RequestHandler() = default;

        virtual void handleRequest(HTTP2ServerRequest & request, HTTP2ServerResponse & response) = 0;
    };

}