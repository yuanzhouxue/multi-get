#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include "HTTPResponse.h"
#include "Pool.h"

namespace multi_get {
class HTTPConnection {
  public:
    HTTPConnection() {
        initHeaders();
    }

    explicit HTTPConnection(const std::string &proxy) : HTTPConnection() {
        if (!proxy.empty())
            this->proxy = proxy;
    };

    virtual HTTPResponse get(const std::string &url);
    virtual HTTPResponse head(const std::string &url);
    void setHeader(const std::string &key, const std::string &val);
    void setProxy(const std::string &_proxy);

  protected:
    Headers headers;
    std::string proxy;
    std::string constructHeaders(const std::string &path, const std::string &method = "GET") const noexcept;
    static HTTPResponse receiveHTTPHeaders(const std::shared_ptr<Connection> &conn);
    static HTTPResponse receiveHTTPHeaders(const PoolGuard &guard) {
        return receiveHTTPHeaders(guard.get());
    }
    void initHeaders() noexcept;
};
} // namespace multi_get

#endif