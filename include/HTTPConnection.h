#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "HTTPResponse.h"

namespace multi_get {
class HTTPConnection {
  public:
    HTTPConnection() {
        initHeaders();
    }

    HTTPResponse get(const std::string &url);
    HTTPResponse head(const std::string &url);
    void setHeader(const std::string &key, const std::string &val);

    virtual ~HTTPConnection();

  private:
    std::vector<char> res;
    std::string req;
    Headers headers;
    std::unordered_map<std::string, int> socks;

    std::string constructHeaders(const std::string &path, const std::string &method = "GET") const noexcept;
    HTTPResponse receiveHTTPHeaders(int socket) const;
    bool connect(const std::string &host);
    void initHeaders() noexcept;
};
} // namespace multi_get

#endif