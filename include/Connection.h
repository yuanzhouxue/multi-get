#ifndef CONNECTION_H
#define CONNECTION_H

#include <string>
#include <cstring>
#include <csignal>
#include <cstdlib>



#ifdef _WIN32

#include <winsock2.h>
#include <WS2tcpip.h>
using ssize_t = SSIZE_T;
inline void close(SOCKET sock) {
    ::closesocket(sock);
}

#elif __linux__
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

using SOCKET = int;

#else
    #error "Unsupported platform"
#endif



namespace multi_get {

class Connection {
  protected:
    SOCKET sock;
    bool _connected{false};

    SOCKET openClientFd(const std::string& host, uint16_t port) {
        SOCKET clientFd;
        addrinfo hints, *listp, *p;
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG;
        std::string portStr = std::to_string(port);
        ::getaddrinfo(host.c_str(), portStr.c_str(), &hints, &listp);

        for (p = listp; p; p = p->ai_next) {
            if ((clientFd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
                continue;
            }
            if (::connect(clientFd, p->ai_addr, p->ai_addrlen) != -1) {
                break;
            }
            ::close(clientFd);
        }
        ::freeaddrinfo(listp);
        if (!p) return -1;
        else return clientFd;
    }

    virtual std::pair<std::string, uint16_t> getHostAndPort(const std::string& host) {
        auto idx = host.find(':');
        uint16_t port = 80;
        std::string realhost = host;
        if (idx != std::string::npos) {
            port = std::stoi(host.substr(idx + 1));
            realhost = host.substr(0, idx);
        }
        return {realhost, port};
    }

  public:
    const bool connected() const noexcept {
        return _connected;
    }
    // 建立socket连接
    virtual bool connect(const std::string &host) {
        if (_connected)
            return true;

        
        const auto& [hostAddr, hostPort] = getHostAndPort(host);

        sock = openClientFd(hostAddr, hostPort);
        if (sock == -1) {
            ::perror("Connection Error: ");
            _connected = false;
            return false;
        }
        _connected = true;
        return true;
    }

    virtual ssize_t send(const char *__buf, size_t __n) const = 0;
    virtual ssize_t receive(char *__buf, size_t __n) const = 0;

    Connection() = default;
    // Connection(const std::string &host, uint16_t port) : host(host), port(port){};

    virtual ~Connection() {
        if (_connected)
            ::close(sock);
    }
};

} // namespace multi_get

#endif