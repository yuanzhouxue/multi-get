



#include "SSLConnection.h"
#include "Logger.h"



namespace multi_get {
    
SSLInitializer::SSLInitializer() {
#if OPENSSL_VERSION_NUMBER >= 0x10100003L
    if (::OPENSSL_init_ssl(OPENSSL_INIT_LOAD_CONFIG, nullptr) == 0) {
        LOG_ERROR("Error initializing SSL connection.");
    }
#else
    OPENSSL_config(NULL);
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
#endif
}

SSLConnection::SSLConnection(const std::string& host, Port port) : Connection(host, port) {
    
}

SSLConnection::SSLConnection(const std::string& host, Port port, const std::string& proxyAddr) : Connection(host, port, proxyAddr) {
}

SSLConnection::~SSLConnection() {
    if (!ssl) {
        return;
    }
    ::SSL_CTX_free(ctx);
    ::SSL_shutdown(ssl);
    ::SSL_free(ssl);
}

ssize_t SSLConnection::send(const char *_buf, size_t _n) const {
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

ssize_t SSLConnection::receive(char *_buf, size_t _n) const {
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
    if (Connection::connected()) {
        return true;
    }
    
    if (!Connection::connect()) {
        return false;
    }

    SSLInitializer::initialize();
    if (!ssl) {
        const SSL_METHOD *m = ::TLS_client_method();
        ctx = ::SSL_CTX_new(m);
        ssl = ::SSL_new(ctx);
        if (!ssl) {
            LOG_ERROR("Error creating SSL context.");
            std::cerr << "Error creating SSL context." << std::endl;
            return false;
        }
    }
    ::SSL_set_fd(ssl, static_cast<int>(socket->getSocketFD()));

    int err = ::SSL_connect(ssl);
    if (err <= 0) {
        auto error = ::SSL_get_error(ssl, err);
        LOG_ERROR("Error creating SSL connection: %d", error);
        std::cerr << "Error creating SSL connection: " << error << std::endl;
        return false;
    }
    return true;
}

}
