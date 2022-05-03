#ifndef HTTPSCONNECTION_H
#define HTTPSCONNECTION_H

#include "HTTPConnection.h"
#include "SSLConnection.h"

namespace multi_get {
class HTTPSConnection : public HTTPConnection {
  public:
    HTTPSConnection() : HTTPConnection(42) {
        conn = new SSLConnection();
    }

    // HTTPSConnection(const std::string& url) : HTTPConnection(42) {
    //     auto dashIdx = url.find('/', 8);
    //     std::string host = url.substr(8, dashIdx - 8);
    //     HTTPSConnection(host, 443);
    // }

    virtual HTTPResponse head(const std::string &url) override;
    virtual HTTPResponse get(const std::string &url) override;
};
} // namespace multi_get

#endif