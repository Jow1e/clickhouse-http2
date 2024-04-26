#pragma once

#include "HTTP2RequestHandlerFactory.h"

#include "Poco/Net/TCPServer.h"
#include "Poco/Net/TCPServerConnection.h"
#include "Poco/Net/TCPServerConnectionFactory.h"

namespace DB {

    class HTTP2Server : public Poco::Net::TCPServer {
    public:
        explicit HTTP2Server(
            HTTP2RequestHandlerFactoryPtr factory,
            Poco::Net::ServerSocket & socket);

        ~HTTP2Server() override;

        void stopAll(bool abort_current = false);

    private:
        HTTP2RequestHandlerFactoryPtr factory;
    };

}
