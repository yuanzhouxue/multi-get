#include "Connection.h"
#include <vector>

namespace multi_get {

ssize_t SSLConnection::send(const char *_buf, size_t _n, int) const {
    auto len = ::SSL_write(ssl, _buf, int(_n));
    if (len < 0) {
        int err = ::SSL_get_error(ssl, len);
        if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ) {
            return 0;
        } else {
            return -1;
        }
    }
    return len;
}

ssize_t SSLConnection::receive(char *_buf, size_t _n, int) const {
    auto len = ::SSL_read(ssl, _buf, int(_n));
    if (len < 0) {
        int err = ::SSL_get_error(ssl, len);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            return 0;
        } else if (err == SSL_ERROR_ZERO_RETURN || err == SSL_ERROR_SYSCALL || err == SSL_ERROR_SSL) {
            return -1;
        }
    }
    return len;
}

bool SSLConnection::connect() {
    if (!Connection::connect()) {
        return false;
    }

    SSLInitializer::initialize();
    if (!ssl) {
        const SSL_METHOD *m = ::TLS_client_method();
        ctx = ::SSL_CTX_new(m);
        ssl = ::SSL_new(ctx);
        if (!ssl) {
            std::cerr << "Error creating SSL context." << std::endl;
            _connected = false;
            return false;
        }
    }
    ::SSL_set_fd(ssl, sock);

    int err = ::SSL_connect(ssl);
    if (err <= 0) {
        auto error = ::SSL_get_error(ssl, err);
        std::cerr << "Error creating SSL connection: " << error << std::endl;
        _connected = false;
        return false;
    }
    _connected = true;
    return true;
}

std::tuple<std::string, std::string, uint16_t> formatHost(const std::string &url) {
    std::string protocol;
    std::string hostname;
    uint16_t port = 0;

    size_t beginIdx;
    if (url.substr(0, 7) == "http://") {
        protocol = "http";
        beginIdx = 7;
    } else if (url.substr(0, 8) == "https://") {
        protocol = "https";
        beginIdx = 8;
    } else {
        std::cerr << "Unsupported url: " << url << std::endl;
        exit(EXIT_FAILURE);
    }

    auto hostPart = url.substr(beginIdx, url.find_first_of('/', beginIdx) - beginIdx);
    if (auto idx = hostPart.find_first_of(':');
        idx != std::string::npos) {
        hostname = hostPart.substr(0, idx);
        port = std::stoi(hostPart.substr(idx + 1));
    } else {
        hostname = hostPart;
        port = protocol == "https" ? 443 : 80;
    }

    return {protocol, hostname, port};
}

bool Connection::do_proxy_handshake() const {
    char req[3] = {0x05, 0x01, 0x00};
    ::send(sock, &req, sizeof(req), 0);
    char res[2];
    ::recv(sock, res, 2, 0);

    if (res[0] != 0x05) {
        std::cerr << "Proxy server is not a socks5 server." << std::endl;
        return false;
    }
    if (res[1] != 0) {
        std::cerr << "Proxy server doer not support no-auth." << std::endl;
        return false;
    }

    std::vector<uint8_t> connectReq;
    connectReq.push_back(0x05); // socks version
    connectReq.push_back(0x01); // cmd: connect
    connectReq.push_back(0x00); // RSV
    connectReq.push_back(0x03); // address type: domain name
    connectReq.push_back(static_cast<uint8_t>(hostname.length())); // domain length
    connectReq.insert(connectReq.end(), hostname.begin(), hostname.end()); // domain name

    PORT p{0, 0};
    p.port_short = ::htons(port);
    connectReq.push_back(p.port_chars[0]); // port
    connectReq.push_back(p.port_chars[1]); // port

    ::send(sock, connectReq.data(), connectReq.size(), 0);
    ::recv(sock, connectReq.data(), connectReq.size(), 0);
    if (connectReq[1] != 0  && connectReq[1] < sizeof(SOCKS_ERROR_STR) / sizeof(const char* const)) {
        std::cerr << SOCKS_ERROR_STR[connectReq[1]] << std::endl;
        return false;
    }
    return true;
}
void Connection::setProxy(const std::string &proxyStr) {
    if (proxyStr.substr(0, 9) != "socks5://") return;

    bool proxyAddrParsed = false;
    std::string p_addr;
    uint16_t p_port = 0;
    for (int i = 9; i < proxyStr.length(); ++i) {
        if (proxyStr[i] == ':') {
            proxyAddrParsed = true;
            continue;
        }
        if (!proxyAddrParsed) {
            p_addr.push_back(proxyStr[i]);
        } else {
            if (std::isdigit(proxyStr[i])) {
                p_port = p_port * 10 + proxyStr[i] - '0';
            } else {
                std::cerr << "Unsupported proxy server port." << std::endl;
                return;
            }
        }
    }
    proxyAddr = move(p_addr);
    proxyPort = p_port;
}
} // namespace multi_get