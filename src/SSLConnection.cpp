#include "SSLConnection.h"

namespace multi_get {

std::pair<std::string, uint16_t> SSLConnection::getHostAndPort(const std::string &host) {
    auto idx = host.find(':');
    uint16_t port = 443;
    std::string realhost = host;
    if (idx != std::string::npos) {
        port = std::stoi(host.substr(idx + 1));
        realhost = host.substr(0, idx);
    }
    return {realhost, port};
}

ssize_t SSLConnection::send(const void *__buf, size_t __n) const {
    int len = SSL_write(ssl, __buf, __n);
    if (len < 0) {
        int err = SSL_get_error(ssl, len);
        if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ) {
            return 0;
        } else {
            return -1;
        }
    }
    return len;
}

ssize_t SSLConnection::receive(void *__buf, size_t __n) const {
    int len = SSL_read(ssl, __buf, __n);
    if (len < 0) {
        int err = SSL_get_error(ssl, len);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            return 0;
        } else if (err == SSL_ERROR_ZERO_RETURN || err == SSL_ERROR_SYSCALL || err == SSL_ERROR_SSL) {
            return -1;
        }
    }
    return len;
}

bool SSLConnection::connect(const std::string &host) {
    if (!Connection::connect(host)) {
        return false;
    }

#if OPENSSL_VERSION_NUMBER >= 0x10100003L
    if (OPENSSL_init_ssl(OPENSSL_INIT_LOAD_CONFIG, NULL) == 0) {
        std::cerr << "Error creating SSL connection." << std::endl;
        _connected = false;
        return false;
    }
#else
    OPENSSL_config(NULL) l
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
#endif
    const SSL_METHOD *m = TLS_client_method();
    SSL_CTX *ctx = SSL_CTX_new(m);
    ssl = SSL_new(ctx);
    if (!ssl) {
        std::cerr << "Error creating SSL connection." << std::endl;
        _connected = false;
        return false;
    }

    SSL_set_fd(ssl, sock);
    // auto ssl_sock = SSL_get_fd(ssl);

    int err = SSL_connect(ssl);
    if (err <= 0) {
        auto error = SSL_get_error(ssl, err);
        std::cerr << "Error creating SSL connection: " << err << std::endl;
        _connected = false;
        return false;
    }
    _connected = true;
    return true;
}

} // namespace multi_get