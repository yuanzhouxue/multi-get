#ifndef PLAIN_CONNECTION_H
#define PLAIN_CONNECTION_H

#include "Connection.h"

namespace multi_get {
class PlainConnection : public Connection {
  public:
    // PlainConnection(const std::string &host, uint16_t port) : Connection(host, port){};
    virtual ssize_t send(const void *__buf, size_t __n) const override {
        return ::send(sock, __buf, __n, 0);
    }

    virtual ssize_t receive(void *__buf, size_t __n) const override {
        return ::recv(sock, __buf, __n, 0);
    }
};
} // namespace multi_get

#endif