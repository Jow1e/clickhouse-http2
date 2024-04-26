#pragma once

#include "HTTP2RequestHandler.h"
#include "HTTP2ServerRequest.h"
#include "HTTP2RequestHandler.h"

#include <memory>

namespace DB
{

    class HTTP2RequestHandlerFactory
    {
    public:
        virtual ~HTTP2RequestHandlerFactory() = default;

        virtual std::unique_ptr<HTTP2RequestHandler> createRequestHandler(const HTTP2ServerRequest & request) = 0;
    };

    using HTTP2RequestHandlerFactoryPtr = std::shared_ptr<HTTP2RequestHandlerFactory>;

    template<class S>
    class HTTP2RequestHandlerFactoryImpl : public HTTP2RequestHandlerFactory
    {
    public:
        HTTP2RequestHandlerFactoryImpl() = default;

        ~HTTP2RequestHandlerFactoryImpl() override = default;

        std::unique_ptr<HTTP2RequestHandler> createRequestHandler(const HTTP2ServerRequest & request) override {
            return std::make_unique<S>();
        }
    };

}
