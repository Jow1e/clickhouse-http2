#include "nghttp2/nghttp2.h"

#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/SocketReactor.h"
#include "Poco/Net/SocketNotification.h"
#include "Poco/NObserver.h"
#include "Poco/Net/StreamSocket.h"
#include <Poco/Thread.h>

#include <iostream>


#define ARRLEN(x) (sizeof(x) / sizeof(x[0]))

#define MAKE_NV(NAME, VALUE, VALUELEN)		                                     \
	{	                                                                          \
		(uint8_t *)NAME, (uint8_t *)VALUE, sizeof(NAME) - 1, VALUELEN,             \
		    NGHTTP2_NV_FLAG_NONE                                                   \
	}

#define MAKE_NV2(NAME, VALUE)		                                              \
	{	                                                                          \
		(uint8_t *)NAME, (uint8_t *)VALUE, sizeof(NAME) - 1, sizeof(VALUE) - 1,    \
		    NGHTTP2_NV_FLAG_NONE                                                   \
	}


class HTTP2Connection {
public:
    Poco::Net::StreamSocket socket;
    Poco::Net::SocketReactor reactor;

    nghttp2_session *session;
    int32_t stream_id;

    HTTP2Connection(const Poco::Net::SocketAddress& sa) : socket(sa) {
        reactor.addEventHandler(socket, Poco::NObserver<HTTP2Connection, Poco::Net::ReadableNotification>(*this, &HTTP2Connection::OnSocketReadable));
        reactor.addEventHandler(socket, Poco::NObserver<HTTP2Connection, Poco::Net::WritableNotification>(*this, &HTTP2Connection::OnSocketWritable));
        reactor.addEventHandler(socket, Poco::NObserver<HTTP2Connection, Poco::Net::ShutdownNotification>(*this, &HTTP2Connection::OnSocketShutdown));
        reactor.addEventHandler(socket, Poco::NObserver<HTTP2Connection, Poco::Net::ErrorNotification>(*this, &HTTP2Connection::OnSocketError));
        reactor.addEventHandler(socket, Poco::NObserver<HTTP2Connection, Poco::Net::TimeoutNotification>(*this, &HTTP2Connection::OnSocketTimeout));

        initialize_nghttp2_session();
        send_client_connection_header();
        submit_request();
        if (session_send() != 0) {
            nghttp2_session_del(session);
        }
    }

    void initialize_nghttp2_session() {
        nghttp2_session_callbacks *callbacks;
        nghttp2_session_callbacks_new(&callbacks);

        nghttp2_session_callbacks_set_send_callback2(callbacks, send_callback);
        nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, on_frame_recv_callback);
        nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, on_data_chunk_recv_callback);
        nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, on_stream_close_callback);
        nghttp2_session_callbacks_set_on_header_callback(callbacks, on_header_callback);
        nghttp2_session_callbacks_set_on_begin_headers_callback(callbacks, on_begin_headers_callback);
        
        nghttp2_session_client_new(&session, callbacks, this);
        nghttp2_session_callbacks_del(callbacks);
    }

    static nghttp2_ssize send_callback(nghttp2_session *session,
                                           const uint8_t *data, size_t length,
                                           int flags, void *user_data) {
        auto *conn = (HTTP2Connection *)user_data;
        conn->socket.sendBytes(data, length);
        return (nghttp2_ssize)length;
    }

    static int on_header_callback(nghttp2_session *session,
                                      const nghttp2_frame *frame, const uint8_t *name,
                                      size_t namelen, const uint8_t *value,
                                      size_t valuelen, uint8_t flags, void *user_data) {
        auto *conn = (HTTP2Connection *)user_data;

        switch (frame->hd.type) {
        case NGHTTP2_HEADERS:
            if (frame->headers.cat == NGHTTP2_HCAT_RESPONSE && conn->stream_id == frame->hd.stream_id) {
              /* Print response headers for the initiated request. */
              std::cerr << name << ": " << value << std::endl;
              break;
            }
        }
        return 0;
    }


    static int on_begin_headers_callback(nghttp2_session *session,
                                             const nghttp2_frame *frame,
                                             void *user_data) {
        auto *conn = (HTTP2Connection *)user_data;

        switch (frame->hd.type) {
        case NGHTTP2_HEADERS:
            if (frame->headers.cat == NGHTTP2_HCAT_RESPONSE && conn->stream_id == frame->hd.stream_id) {
              std::cerr << "[Client]: Response headers for stream ID=" << frame->hd.stream_id << std::endl;
            }
            break;
        }
        return 0;
    }

    static int on_frame_recv_callback(nghttp2_session *session,
                                          const nghttp2_frame *frame, void *user_data) {
        auto *conn = (HTTP2Connection *)user_data;

        switch (frame->hd.type) {
        case NGHTTP2_HEADERS:
            if (frame->headers.cat == NGHTTP2_HCAT_RESPONSE && conn->stream_id == frame->hd.stream_id) {
                std::cerr << "[Client]: All headers received" << std::endl;
            }
            break;
        }
        return 0;
    }

    static int on_data_chunk_recv_callback(nghttp2_session *session, uint8_t flags,
                                               int32_t stream_id, const uint8_t *data,
                                               size_t len, void *user_data) {
        auto *conn = (HTTP2Connection *)user_data;

        if (conn->stream_id == stream_id) {
            fwrite(data, 1, len, stdout);
        }
        return 0;
    }

    static int on_stream_close_callback(nghttp2_session *session, int32_t stream_id,
                                            uint32_t error_code, void *user_data) {
        auto *conn = (HTTP2Connection *)user_data;
        int rv;

        if (conn->stream_id == stream_id) {
            std::cerr << "[Client]: Stream " << stream_id << " closed with error_code=" << error_code << std::endl;
            rv = nghttp2_session_terminate_session(session, NGHTTP2_NO_ERROR);
            if (rv != 0) {
              return NGHTTP2_ERR_CALLBACK_FAILURE;
            }
        }
        return 0;
    }

    void send_client_connection_header() {
        nghttp2_settings_entry iv[1] = {{NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}};
        int rv = nghttp2_submit_settings(session, NGHTTP2_FLAG_NONE, iv, ARRLEN(iv));
        if (rv != 0) {
            std::cerr << "[Client]: Could not submit SETTINGS: " << nghttp2_strerror(rv) << std::endl;
        }
    }

    static void print_headers(nghttp2_nv *nva, size_t nvlen) {
        for (size_t i = 0; i < nvlen; ++i) {
            std::cerr << nva[i].name << ": ";
            std::cerr << nva[i].value << '\n';
        }

        std::cerr << std::endl;
    }

    void submit_request() {
        nghttp2_nv hdrs[] = {
              MAKE_NV2(":method", "GET"),
              MAKE_NV2(":scheme", "http"),
              MAKE_NV2(":path", "/")
        };

        std::cerr << "[Client]: Request headers:" << std::endl;
        print_headers(hdrs, ARRLEN(hdrs));

        stream_id = nghttp2_submit_request2(session, NULL, hdrs, ARRLEN(hdrs), NULL, &stream_id);
        if (stream_id < 0) {
            std::cerr << "Could not submit HTTP request: %s" << nghttp2_strerror(stream_id) << std::endl;
        }
    }

    int session_send() {
        int rv;

        rv = nghttp2_session_send(session);
        if (rv != 0) {
            std::cerr << "[Client]: Fatal error: " << nghttp2_strerror(rv) << std::endl;
            return -1;
        }

        return 0;
    }

    ~HTTP2Connection() {
        nghttp2_session_del(session);
        Close();
    }

    void Run() {
        reactor.run();
    }

    void Stop() { reactor.stop(); }

    void OnSocketReadable(const Poco::AutoPtr<Poco::Net::ReadableNotification>& pNf) {
        uint8_t data[10000];
        int datalen = socket.receiveBytes(data, ARRLEN(data));
        if (datalen == 0) {
            std::cerr << "[Client]: Shutdown from peer" << std::endl;
            return;
        }

        nghttp2_ssize readlen = nghttp2_session_mem_recv2(session, data, datalen);
        if (readlen < 0) {
            std::cerr << "[Client]: Fatal error: " << nghttp2_strerror((int)readlen);
        }
        
        if (session_send() != 0) {
            nghttp2_session_del(session);
        }
    }

    void OnSocketWritable(const Poco::AutoPtr<Poco::Net::WritableNotification>& pNf) {
        // should also check for some output size, idk what
        if (nghttp2_session_want_read(session) == 0 && nghttp2_session_want_write(session) == 0) {
            nghttp2_session_del(session);
        }
    }

    void OnSocketShutdown(const Poco::AutoPtr<Poco::Net::ShutdownNotification>& pNf) {
        nghttp2_session_del(session);
    }

    void OnSocketError(const Poco::AutoPtr<Poco::Net::ErrorNotification>& pNf) {
        std::cerr << "[Client]: Socket error" << std::endl;
    }

    void OnSocketTimeout(const Poco::AutoPtr<Poco::Net::TimeoutNotification>& pNf) {
        std::cerr << "[Client]: Socket timeout. " << pNf->name() << std::endl;
    }

    void Close() {
        try {
            socket.shutdown();
        }
        catch (...) {
            std::cerr << "[Client]: Failed to close socket." << std::endl;
        }
    }
};


int main() {
    HTTP2Connection conn(Poco::Net::SocketAddress("localhost", 8080));
    conn.Run();
}
