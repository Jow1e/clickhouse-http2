#include "HTTP2Server.h"

#include "HTTP2ServerConnectionFactory.h"

namespace DB
{
    HTTP2Server::HTTP2Server(
            HTTP2RequestHandlerFactoryPtr factory_,
            Poco::Net::ServerSocket & socket_)
            : Poco::Net::TCPServer(new HTTP2ServerConnectionFactory(factory_), socket_), factory(factory_)
    {
    }

    HTTP2Server::~HTTP2Server()
    {
        /// We should call stop and join thread here instead of destructor of parent TCPHandler,
        /// because there's possible race on 'vptr' between this virtual destructor and 'run' method.
        stop();
    }

    void HTTP2Server::stopAll(bool /* abortCurrent */)
    {
        stop();
    }

}


