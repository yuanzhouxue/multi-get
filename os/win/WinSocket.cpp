#include "WinSocket.h"

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <tuple>
#include <utility>
#include <openssl/ssl.h>
#include "Logger.h"

#pragma comment(lib, "ws2_32.lib")

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

namespace multi_get {

std::unique_ptr<Socket> createSocket(const std::string& host, Port port) {
    return std::make_unique<WinSocket>(host, port);
}

WinSocket::WinSocket(const std::string& host, Port port) : host(host), port(port) {
    sock = ::socket(AF_INET, SOCK_STREAM, 0);
}

bool WinSocket::connected() const {
    return _connected;
}

bool WinSocket::connect() {
    if (_connected) {
        return true;
    }
    SOCKET clientFd;
    addrinfo hints{}, *listp, *p;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG;
    std::string portStr = std::to_string(port);
    if (int s = ::getaddrinfo(host.c_str(), portStr.c_str(), &hints, &listp);
        0 != s) {
        LOG_ERROR("getaddrinfo: %s", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    for (p = listp; p; p = p->ai_next) {
        if ((clientFd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            continue;
        }
        if (::connect(clientFd, p->ai_addr, static_cast<int>(p->ai_addrlen)) != -1) {
            break;
        }
        ::closesocket(clientFd);
    }
    ::freeaddrinfo(listp);
    if (!p) {
        ::perror("Socket Connection Error");
        exit(EXIT_FAILURE);
    }
    sock = clientFd;
    _connected = true;
    return true;
}

WinSocket::~WinSocket() noexcept {
    doDisconnect();
}

void WinSocket::disconnect() {
    doDisconnect();
}

void WinSocket::doDisconnect() const {
    ::closesocket(sock);
}

int WinSocket::getSocketFD() const {
    return static_cast<int>(sock);
}

ssize_t WinSocket::send(const char *_buf, size_t _n) const {
    return ::send(sock, _buf, _n, 0);
}

ssize_t WinSocket::receive(char *_buf, size_t _n) const {
    return ::recv(sock, _buf, _n, 0);
}

Port htons(Port p) {
    return ::htons(p);
}

}