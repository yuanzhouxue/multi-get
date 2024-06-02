#ifndef __WINSOCKET_H
#define __WINSOCKET_H

#include "Socket.h"

namespace multi_get {

class WinSocket: public Socket {
public:
    WinSocket(const std::string& host, Port port);
    bool connected() const override;
    bool connect() override;
    void disconnect() override;
    ssize_t send(const char *_buf, size_t _n) const override;
    ssize_t receive(char *_buf, size_t _n) const override;
    int getSocketFD() const override;
    ~WinSocket();

private:
    std::string host;
    Port port;
    bool _connected{false};
    SOCKET sock;

};

}


#endif