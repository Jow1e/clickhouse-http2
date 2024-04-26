#include "HTTP2RequestHandler.h"

namespace DB {

    class HTTP2StaticHandler : public HTTP2RequestHandler {
        void handleRequest(HTTP2ServerRequest & request, HTTP2ServerResponse & response);
    };

} // DB
