#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include "PlainConnection.h"
#include "HTTPResponse.h"

namespace multi_get {
class HTTPConnection {
  public:
    HTTPConnection() : conn(new PlainConnection()) {
        initHeaders();
    }

    virtual HTTPResponse get(const std::string &url);

    virtual HTTPResponse head(const std::string &url);

    void setHeader(const std::string &key, const std::string &val);

    virtual ~HTTPConnection() {
        delete conn;
    };

  protected:
    Connection* conn;
    std::vector<char> res;
    std::string req;
    Headers headers;
    std::string constructHeaders(const std::string &path, const std::string &method = "GET") const noexcept;
    HTTPResponse receiveHTTPHeaders() const;
    void initHeaders() noexcept;

    HTTPResponse head(const std::string& host, const std::string& fileUrl);
    HTTPResponse get(const std::string& host, const std::string& fileUrl);
    HTTPConnection(int) {
        initHeaders();
    }
};
} // namespace multi_get

#endif