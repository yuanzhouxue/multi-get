#include "Connection.h"

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

} // namespace multi_get