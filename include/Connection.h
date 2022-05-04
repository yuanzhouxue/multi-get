#ifndef CONNECTION_H
#define CONNECTION_H

#include <string>
#include <cstring>
#include <csignal>
#include <cstdlib>



#ifdef _WIN32

#include <winsock2.h>
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

    struct sockaddr_in constructSockaddr(const std::string &host, uint16_t port) {
        auto servIP = ::gethostbyname(host.c_str());
        struct sockaddr_in servAddr;
        std::memset(&servAddr, 0, sizeof(servAddr));
        servAddr.sin_family = AF_INET;
        servAddr.sin_addr.s_addr = ::inet_addr(inet_ntoa(*(struct in_addr *)servIP->h_addr_list[0]));
        servAddr.sin_port = ::htons(port);
        return servAddr;
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
        sock = ::socket(PF_INET, SOCK_STREAM, 0);
        auto servAddr = constructSockaddr(hostAddr, hostPort);

        auto err = ::connect(sock, (struct sockaddr *)&servAddr, sizeof(servAddr));
        if (err != 0) {
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