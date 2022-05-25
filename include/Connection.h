#ifndef CONNECTION_H
#define CONNECTION_H

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <tuple>

#include <openssl/ssl.h>

#ifdef _WIN32

#include <WS2tcpip.h>
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
    union PORT {
        uint8_t port_chars[2];
        uint16_t port_short;
    };
    constexpr static const char *const SOCKS_ERROR_STR[] = {
        "Succeed",
        "General SOCKS server failure",
        "Connection not allowed by ruleset",
        "Network unreachable",
        "Host unreachable",
        "Connection refused",
        "TTL expired",
        "Command not supported",
        "Address type not supported"};

    SOCKET sock{};
    bool _connected{false};
    std::string hostname;
    uint16_t port{0};
    std::string proxyAddr;
    uint16_t proxyPort{0};

    static SOCKET openClientFd(const std::string &hostname, uint16_t port) {
        SOCKET clientFd;
        addrinfo hints{}, *listp, *p;
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG;
        std::string portStr = std::to_string(port);
        if (int s = ::getaddrinfo(hostname.c_str(), portStr.c_str(), &hints, &listp);
            0 != s) {
            fprintf(stderr, "getaddrinfo: %s", gai_strerror(s));
            exit(EXIT_FAILURE);
        }

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
        if (!p) {
            ::perror("Connection Error");
            exit(EXIT_FAILURE);
        }
        return clientFd;
    }

  public:
    [[nodiscard]] bool connected() const noexcept {
        return _connected;
    }
    // 建立socket连接
    virtual bool connect() {
        if (_connected)
            return true;

        //        const auto &[hostAddr, hostPort] = getHostAndPort(host);
        if (proxyAddr.empty()) {
            sock = openClientFd(hostname, port);
            _connected = true;
        } else {
            sock = openClientFd(proxyAddr, proxyPort);
            _connected = do_proxy_handshake();
        }
        //        struct timeval timeout = {3, 0};
        //        setoption(SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval));
        return _connected;
    }

    virtual ssize_t send(const char *_buf, size_t _n, int _flag = 0) const = 0;
    virtual ssize_t receive(char *_buf, size_t _n, int _flag = 0) const = 0;

    void setoption(int _level, int _optname, const char *_optval, socklen_t _optlen) noexcept {
        ::setsockopt(this->sock, _level, _optname, _optval, _optlen);
    }

    bool do_proxy_handshake() const;
    void setProxy(const std::string &proxyStr);

    Connection() = default;
    Connection(const std::string &hostname, uint16_t port) : hostname(hostname), port(port){};
    Connection(const std::string &hostname, uint16_t port, const std::string &proxy) : Connection(hostname, port) {
        setProxy(proxy);
    }

    virtual ~Connection() {
        if (_connected)
            ::close(sock);
    }
};

class PlainConnection : public Connection {
  public:
    PlainConnection(const std::string &hostname, uint16_t port) : Connection(hostname, port){};
    PlainConnection(const std::string &hostname, uint16_t port, const std::string& proxy) : Connection(hostname, port, proxy){};
    ssize_t send(const char *_buf, size_t _n, int _flag) const override {
        return ::send(sock, _buf, _n, _flag);
    }

    ssize_t receive(char *_buf, size_t _n, int _flag) const override {
        return ::recv(sock, _buf, _n, _flag);
    }
};

class SSLInitializer {
  private:
    SSLInitializer() {
#if OPENSSL_VERSION_NUMBER >= 0x10100003L
        if (::OPENSSL_init_ssl(OPENSSL_INIT_LOAD_CONFIG, nullptr) == 0) {
            std::cerr << "Error initializing SSL connection." << std::endl;
        }
#else
        OPENSSL_config(NULL);
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
#endif
    };

  public:
    ~SSLInitializer() = default;
    SSLInitializer(const SSLInitializer &) = delete;
    SSLInitializer &operator=(const SSLInitializer &) = delete;

  public:
    [[maybe_unused]] static void initialize() {
        static SSLInitializer instance;
    }
};

class SSLConnection : public Connection {
  private:
    SSL *ssl{};
    SSL_CTX *ctx{};

  protected:
    ssize_t send(const char *_buf, size_t _n, int _flag) const override;

    ssize_t receive(char *_buf, size_t _n, int _flag) const override;

    bool connect() override;

  public:
    SSLConnection(const std::string &hostname, uint16_t port) : Connection(hostname, port){};
    SSLConnection(const std::string &hostname, uint16_t port, const std::string& proxy) : Connection(hostname, port, proxy){};
    ~SSLConnection() override {
        if (ssl) {
            ::SSL_CTX_free(ctx);
            ::SSL_shutdown(ssl);
            ::SSL_free(ssl);
        }
    }
};

std::tuple<std::string, std::string, uint16_t> formatHost(const std::string &url);

} // namespace multi_get

#endif