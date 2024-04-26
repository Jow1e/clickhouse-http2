#include "HTTP2ServerConnection.h"

namespace DB {

    static int on_begin_headers_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data) {
        auto http2_connection = static_cast<HTTP2ServerConnection*>(user_data);

        if (frame->hd.type != NGHTTP2_HEADERS ||
            frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
            return 0;
        }

        http2_connection->createStream(frame->hd.stream_id);

        return 0;
    }

    static int on_header_callback(nghttp2_session *session,
                                  const nghttp2_frame *frame, const uint8_t *name,
                                  size_t namelen, const uint8_t *value,
                                  size_t valuelen, uint8_t flags,
                                  void *user_data) {
        auto http2_connection = static_cast<HTTP2ServerConnection*>(user_data);
        auto stream_id = frame->hd.stream_id;

        if (frame->hd.type != NGHTTP2_HEADERS ||
            frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
            return 0;
        }

        auto stream = http2_connection->getStream(stream_id);
        if (!stream) {
            return 0;
        }

        stream->request.addHeader(name, namelen, value, valuelen);

        return 0;
    }

    int on_frame_recv_callback(nghttp2_session *session, const nghttp2_frame *frame,
                               void *user_data) {
        auto http2_connection = static_cast<HTTP2ServerConnection*>(user_data);
        auto stream = http2_connection->getStream(frame->hd.stream_id);

        if (!stream) {
            return 0;
        }

        switch (frame->hd.type) {
            case NGHTTP2_DATA:
                break;
            case NGHTTP2_HEADERS: {
                if (frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
                    break;
                }
                if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM) {
                    http2_connection->handleRequest(stream);
                }
                break;
            }
            default:
                break;
        }

        return 0;
    }

    static nghttp2_ssize copyResponseData(nghttp2_session *session, int32_t stream_id,
        uint8_t *buf, size_t length, uint32_t *data_flags, nghttp2_data_source *source,
        void *user_data) {
        auto stream = static_cast<HTTP2ServerStream*>(source->ptr);
        size_t n = stream->response.out.read(buf, length);
        return n;
    };

    int on_stream_close_callback(nghttp2_session *session, int32_t stream_id,
                                 uint32_t error_code, void *user_data) {
        auto http2_connection = static_cast<HTTP2ServerConnection*>(user_data);
        auto stream = http2_connection->getStream(stream_id);

        if (!stream) {
            return 0;
        }

        http2_connection->removeStream(stream_id);
        return 0;
    }

    HTTP2ServerConnection::HTTP2ServerConnection(
            const Poco::Net::StreamSocket & socket,
            HTTP2RequestHandlerFactoryPtr factory)
            : Poco::Net::TCPServerConnection(socket), stream_socket(socket), input_buffer(100000), factory(factory) {}

    void HTTP2ServerConnection::run() {
        initHTTP2Session();

        while (!stopped) {
            int rv = sessionReceive();
            if (rv != 0) {
                //TODO: add connection error handling
                return;
            }
        }
    }

    int HTTP2ServerConnection::initHTTP2Session() {
        int rv;

        nghttp2_session_callbacks *callbacks;
        rv = nghttp2_session_callbacks_new(&callbacks);
        if (rv != 0) {
            nghttp2_session_callbacks_del(callbacks);
            return -1;
        }

        nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks,
                                                             on_frame_recv_callback);
        nghttp2_session_callbacks_set_on_stream_close_callback(
                callbacks, on_stream_close_callback);
        nghttp2_session_callbacks_set_on_header_callback(callbacks,
                                                         on_header_callback);
        nghttp2_session_callbacks_set_on_begin_headers_callback(
                callbacks, on_begin_headers_callback);
//        nghttp2_session_callbacks_set_on_frame_not_send_callback(
//                callbacks, on_frame_not_send_callback);
//        nghttp2_session_callbacks_set_on_data_chunk_recv_callback(
//                callbacks, on_data_chunk_recv_callback);

        rv = nghttp2_session_server_new(&session, callbacks, this);
        if (rv != 0) {
            nghttp2_session_callbacks_del(callbacks);
            return -1;
        }

        if (sendServerConnectionHeader() != 0 ||
            sessionSend() != 0) {
            nghttp2_session_callbacks_del(callbacks);
            return -1;
        }

        nghttp2_session_callbacks_del(callbacks);
        return 0;
    }

    void HTTP2ServerConnection::removeStream(int stream_id) {
        streams.erase(stream_id);
    }

    int HTTP2ServerConnection::sessionReceive() {
        if (!input_buffer.isReadable()) {
            if (input_buffer.isWritable()) {
                std::array<char, buf_input_size> buf_data;
                int n = stream_socket.receiveBytes(buf_data.data(), buf_input_size);
                input_buffer.write(buf_data.data(), n);
            } else {
                return -1;
            }
        }
        ssize_t readlen = nghttp2_session_mem_recv(session, (uint8_t*)input_buffer.begin(), input_buffer.size());
        if (readlen < 0) {
            return -1;
        }

        if (readlen) {
            input_buffer.drain(readlen);
        }

        return sessionSend();
    }

    void HTTP2ServerConnection::handleRequest(HTTP2ServerStream* stream) {
        std::lock_guard lock(mutex);
        if (stopped) {
            return;
        }

        std::unique_ptr<HTTP2RequestHandler> handler(factory->createRequestHandler(stream->request));

        if (handler) {
            handler->handleRequest(stream->request, stream->response);
        } else {
            // not implemented
            return;
        }

    }

    static nghttp2_nv addNV(const std::string& name, const std::string& value) {
        return {(uint8_t *)name.c_str(), (uint8_t *)value.c_str(), name.size(), value.size(),
                NGHTTP2_NV_FLAG_NO_COPY_NAME};
    }

    void HTTP2ServerConnection::sendResponse(DB::HTTP2ServerStream *stream) {
        auto nva = std::vector<nghttp2_nv>();

        for (auto &hd : stream->response.headers) {
            nva.push_back(addNV(hd.first, hd.second));
        }

        nghttp2_data_provider *prd_ptr = nullptr, prd;

        if (!stream->response.out.isEmpty()) {
                prd.source.ptr = &stream;
                prd.read_callback = copyResponseData;
                prd_ptr = &prd;
        }
        int rv = nghttp2_submit_response(session, stream->stream_id, nva.data(),
                                     nva.size(), prd_ptr);

        sessionSend();
    }

    HTTP2ServerConnection::~HTTP2ServerConnection() {
        nghttp2_session_del(session);
    }

    int HTTP2ServerConnection::sendServerConnectionHeader() {
        nghttp2_settings_entry iv[1] = {
                {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}};

        int rv;
        rv = nghttp2_submit_settings(session, NGHTTP2_FLAG_NONE, iv, 1);
        return rv;
    }

    int HTTP2ServerConnection::sessionSend() {
        for(;;) {
            const uint8_t *data_ptr;
            nghttp2_ssize n = nghttp2_session_mem_send(session, &data_ptr);

            if (n < 0) {
                return -1;
            }
            else if (n == 0) {
                return 0;
            }

            stream_socket.sendBytes(data_ptr, static_cast<int>(n));
        }
    }

    void HTTP2ServerConnection::createStream(int stream_id) {
        streams.emplace(stream_id, std::make_unique<HTTP2ServerStream>(stream_id, this));
    }

    HTTP2ServerStream* HTTP2ServerConnection::getStream(int stream_id) {
        if (auto it = streams.find(stream_id); it != streams.end()) {
            return it->second.get();
        }
        return nullptr;
    }
} // DB