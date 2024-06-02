#ifndef __SSL_CONNECTION_H
#define __SSL_CONNECTION_H

#include <openssl/ssl.h>

#include "Connection.h"

namespace multi_get {

class SSLInitializer {
  private:
    SSLInitializer();

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
    ssize_t send(const char *_buf, size_t _n) const override;

    ssize_t receive(char *_buf, size_t _n) const override;

    bool connect() override;

  public:
    SSLConnection(const std::string &hostname, Port port);
    SSLConnection(const std::string &hostname, Port port, const std::string& proxy);
    ~SSLConnection() override;
};
}

#endif
