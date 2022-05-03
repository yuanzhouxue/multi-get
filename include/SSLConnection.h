#ifndef SSL_CONNECTION_H
#define SSL_CONNECTION_H

#include <iostream>

#include "Connection.h"
#include "openssl/ssl.h"

namespace multi_get {
class SSLConnection : public Connection {
  private:
    SSL *ssl;

    virtual std::pair<std::string, uint16_t> getHostAndPort(const std::string &host) override;

  protected:
    ssize_t send(const void *__buf, size_t __n) const override;

    ssize_t receive(void *__buf, size_t __n) const override;

    bool connect(const std::string &host) override;

  public:
    // SSLConnection(const std::string& host, uint16_t port) : Connection(host, port) {};
    virtual ~SSLConnection() {
        if (ssl) {
            SSL_shutdown(ssl);
            SSL_free(ssl);
        }
    }
};
} // namespace multi_get

#endif