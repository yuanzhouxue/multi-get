#include "HTTPSConnection.h"
#include <cstdio>
#include <cstring>

namespace multi_get {

HTTPResponse HTTPSConnection::get(const std::string &url) {
    auto dashIdx = url.find('/', 8);
    std::string host = url.substr(8, dashIdx - 8);
    std::string path = dashIdx == std::string::npos ? "/" : url.substr(dashIdx);
    
    return HTTPConnection::get(host, path);
}

HTTPResponse HTTPSConnection::head(const std::string &url) {
    auto dashIdx = url.find('/', 8);
    std::string host = url.substr(8, dashIdx - 8);
    std::string path = dashIdx == std::string::npos ? "/" : url.substr(dashIdx);
    return HTTPConnection::head(host, path);
}

} // namespace multi_get