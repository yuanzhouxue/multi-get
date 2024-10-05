#ifndef __WINSOCKET_H
#define __WINSOCKET_H

#include "Socket.h"
#include <WS2tcpip.h>
#include <winsock2.h>

namespace multi_get {

class WinSocket: public Socket {
public:
    WinSocket(const std::string& host, Port port);
    [[nodiscard]] bool connected() const override;
    bool connect() override;
    void disconnect() override;
    ssize_t send(const char *_buf, size_t _n) const override;
    ssize_t receive(char *_buf, size_t _n) const override;
    [[nodiscard]] int getSocketFD() const override;
    ~WinSocket() override;

private:
    std::string host;
    Port port;
    bool _connected{false};
    SOCKET sock;

    void doDisconnect() const;
};

}


#endif