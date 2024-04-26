#include "HTTP2Server.h"
#include "HTTP2StaticHandler.h"

#include <iostream>
#include <memory>

int main() {
    uint16_t port = 8080;

    // Create the server socket to listen
    Poco::Net::ServerSocket socket(port);

    auto factory = std::make_shared<DB::HTTP2RequestHandlerFactoryImpl<DB::HTTP2StaticHandler>>();
    DB::HTTP2Server server(factory, socket);

    server.start();

    std::cout << "Start listening on port: " << port << "\n";

    while (true)
        ;

    return 0;
}
