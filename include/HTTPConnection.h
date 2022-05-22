#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include "Pool.h"
#include "HTTPResponse.h"

namespace multi_get {
class HTTPConnection {
  public:
    HTTPConnection() {
        initHeaders();
    }

    virtual HTTPResponse get(const std::string &url);
    virtual HTTPResponse head(const std::string &url);
    void setHeader(const std::string &key, const std::string &val);

  protected:
    Headers headers;
    std::string constructHeaders(const std::string &path, const std::string &method = "GET") const noexcept;
    static HTTPResponse receiveHTTPHeaders(const std::shared_ptr<Connection>& conn);
    static HTTPResponse receiveHTTPHeaders(const PoolGuard& guard) {
        return receiveHTTPHeaders(guard.get());
    }
    void initHeaders() noexcept;
};
} // namespace multi_get

#endif