#pragma once

#include "HTTP2RequestHandlerFactory.h"
#include "HTTP2ServerStream.h"

#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/HTTPServerSession.h>
#include <Poco/Net/TCPServerConnection.h>
#include <Poco/FIFOBuffer.h>

#include "nghttp2/nghttp2.h"

#include <mutex>
#include <map>

namespace DB
{

    class HTTP2ServerConnection : public Poco::Net::TCPServerConnection
    {
    public:
        HTTP2ServerConnection(
            const Poco::Net::StreamSocket & socket,
            HTTP2RequestHandlerFactoryPtr factory);

        void run() override;

        void createStream(int stream_id);

        HTTP2ServerStream* getStream(int stream_id);

        void removeStream(int stream_id);

        void handleRequest(HTTP2ServerStream* stream);

        void sendResponse(HTTP2ServerStream* stream);

        ~HTTP2ServerConnection() override;

    private:

        int initHTTP2Session();

        int sendServerConnectionHeader();

        int sessionSend();

        int sessionReceive();

        HTTP2RequestHandlerFactoryPtr factory;
        bool stopped;
        std::mutex mutex;  // guards the |factory| with assumption that creating handlers is not thread-safe.
        nghttp2_session* session;
        Poco::Net::StreamSocket stream_socket;
        Poco::FIFOBuffer input_buffer;
        static constexpr size_t buf_input_size = 8192;
        std::map<int32_t, HTTP2ServerStreamPtr> streams;
    };
}