#include "Socket.h"
#include <string>

namespace multi_get {


using socket_t = int;

class UnixSocket: public Socket {
public:
    UnixSocket(const std::string& host, Port port);
    bool connected() const override;
    bool connect() override;
    void disconnect() override;
    ssize_t send(const char *_buf, size_t _n) const override;
    ssize_t receive(char *_buf, size_t _n) const override;
    int getSocketFD() const override;
    ~UnixSocket();

private:
    std::string host;
    Port port;
    bool _connected{false};
    socket_t sock;
};

}