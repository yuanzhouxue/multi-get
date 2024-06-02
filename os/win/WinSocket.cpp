#include "WinSocket.h"

/** 多系统兼容

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <tuple>
#include <utility>

#include <openssl/ssl.h>

#include "Logger.h"

#ifdef _WIN32

#include <WS2tcpip.h>
#include <winsock2.h>
using ssize_t = SSIZE_T;
using socket_t = SOCKET;
inline void close_socket(socket_t sock) {
    ::closesocket(sock);
}

class WSInit {
    bool is_valid{false};
public:
    WSInit() {
        WSAData wsadata{};
        is_valid = (WSAStartup(MAKEWORD(2, 2), &wsadata) == 0);
    }
    ~WSInit() {
        if (is_valid) {
            WSACleanup();
        }
    }
};

[[maybe_unused]] static WSInit init_ws;

#elif __linux__
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

using socket_t = int;
inline void close_socket(socket_t sock) {
    ::close(sock);
}

#elif __APPLE__
#include <iostream>
// #error "Unsupported platform"
#endif

*/


// #include <WS2tcpip.h>
// #include <winsock2.h>
// using ssize_t = SSIZE_T;
// using socket_t = SOCKET;
// inline void close_socket(socket_t sock) {
//     ::closesocket(sock);
// }

// class WSInit {
//     bool is_valid{false};
// public:
//     WSInit() {
//         WSAData wsadata{};
//         is_valid = (WSAStartup(MAKEWORD(2, 2), &wsadata) == 0);
//     }
//     ~WSInit() {
//         if (is_valid) {
//             WSACleanup();
//         }
//     }
// };

namespace multi_get {


}