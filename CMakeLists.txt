cmake_minimum_required(VERSION 3.27)
project(poco_http2 VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_PREFIX_PATH /home/nikita/libs/lib64/cmake/Poco)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" ${CMAKE_MODULE_PATH})

find_package(Poco REQUIRED Foundation Net)
find_package(Libnghttp2 1.55.1)

foreach(name
        lib/libnghttp2_asio.pc
)

endforeach()

if(Poco_FOUND)
    message ("Poco library found.")
else()
    message (FATAL_ERROR "Cannot find Poco library.")
endif()

message("Libnghttp2:     ${HAVE_LIBNGHTTP2} (LIBS='${LIBNGHTTP2_LIBRARIES}')")

add_executable(poco_http2 main.cpp
        HTTP2Server.cpp
        HTTP2Server.h
        HTTP2RequestHandlerFactory.cpp
        HTTP2RequestHandlerFactory.h
        HTTP2ServerConnectionFactory.cpp
        HTTP2ServerConnectionFactory.h
        HTTP2ServerConnection.cpp
        HTTP2ServerConnection.h
        HTTP2ServerStream.cpp
        HTTP2ServerStream.h
        HTTP2ServerRequest.cpp
        HTTP2ServerRequest.h
        HTTP2ServerResponse.cpp
        HTTP2ServerResponse.h
        HTTP2RequestHandler.cpp
        HTTP2RequestHandler.h
        StaticHandler.cpp
        HTTP2StaticHandler.h
)
target_link_libraries(poco_http2 Poco::Foundation Poco::Net nghttp2)
target_include_directories (poco_http2 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
