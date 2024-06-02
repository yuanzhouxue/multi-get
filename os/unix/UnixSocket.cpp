
#include <memory>
#include <string>
#include <sys/_types/_ssize_t.h>
#include <sys/socket.h>
#include <netdb.h>
#include <format>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Connection.h"
#include "Pool.h"
#include "UnixSocket.h"

namespace multi_get {

using namespace std;


unique_ptr<Socket> createSocket(const std::string &host, Port port) {
    return make_unique<UnixSocket>(host, port);
}

UnixSocket::UnixSocket(const std::string& host, Port port): host(host), port(port) {
    auto sock = ::socket(AF_INET, SOCK_STREAM, 0);
}

bool UnixSocket::connected() const {
    return _connected;
}

bool UnixSocket::connect() {
    if (_connected) {
        return true;
    }

    socket_t clientFd;
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
        ::close(clientFd);
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

UnixSocket::~UnixSocket() {
    ::close(sock);
}

void UnixSocket::disconnect() {
    ::close(sock);
}

int UnixSocket::getSocketFD() const {
    return static_cast<int>(sock);
}

ssize_t UnixSocket::send(const char *_buf, size_t _n) const {
    return ::send(sock, _buf, _n, 0);
}

ssize_t UnixSocket::receive(char *_buf, size_t _n) const {
    return ::recv(sock, _buf, _n, 0);
}

}
