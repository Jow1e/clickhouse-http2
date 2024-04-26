#pragma once

#include "HTTP2RequestHandlerFactory.h"

#include <Poco/Net/TCPServerConnectionFactory.h>

namespace DB
{

class HTTP2ServerConnectionFactory : public Poco::Net::TCPServerConnectionFactory {
    public:
        HTTP2ServerConnectionFactory(HTTP2RequestHandlerFactoryPtr factory);

        Poco::Net::TCPServerConnection * createConnection(const Poco::Net::StreamSocket & socket) override;

    private:
        HTTP2RequestHandlerFactoryPtr factory;
    };

}
