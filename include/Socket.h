#ifndef __SOCKET_H
#define __SOCKET_H

#include <memory>
#include <string>

#include "CommonDefines.h"

namespace multi_get {

class Socket {
public:
    virtual bool connected() const = 0;
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual ssize_t send(const char *_buf, size_t _n) const = 0;
    virtual ssize_t receive(char *_buf, size_t _n) const = 0;

    virtual int getSocketFD() const = 0;
    virtual ~Socket() = default;
};

std::unique_ptr<Socket> createSocket(const std::string& host, Port port);
    
} // namespace multi_get

#endif