#ifndef CONNECTION_H
#define CONNECTION_H

#include <iostream>
#include <memory>
#include <string>

#include "Socket.h"

namespace multi_get {

class Connection {
  private:
    bool doProxyHandshake() const;
  protected:
    std::string hostname;
    Port port{0};
    
    std::string proxyAddr;
    uint16_t proxyPort{0};

    std::unique_ptr<Socket> socket;

  public:
    [[nodiscard]] bool connected() const noexcept;
    // 建立socket连接
    virtual bool connect();

    virtual ssize_t send(const char *_buf, size_t _n) const;
    virtual ssize_t receive(char *_buf, size_t _n) const;
    void receiveNBytes(char* _buf, size_t n) const;

    Connection() = delete;
    Connection(const std::string& hostname, Port port);
    Connection(const std::string& hostname, Port port, const std::string& proxy);

    Connection(const Connection& conn) = delete; // copy constructor
    Connection& operator=(const Connection& conn) = delete; // copy assignment
    
    Connection(Connection&& conn);
    Connection& operator=(Connection&& conn);

    virtual ~Connection();
};


} // namespace multi_get

#endif