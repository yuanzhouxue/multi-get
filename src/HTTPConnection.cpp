#include "HTTPConnection.h"
#include "version.h"
#include <cstdio>
#include <cstring>

namespace multi_get {

HTTPResponse HTTPConnection::get(const std::string &url) {
    auto dashIdx = url.find('/', 7);
    std::string host = url.substr(7, dashIdx - 7);
    std::string path = dashIdx == std::string::npos ? "/" : url.substr(dashIdx);
    return get(host, path);
}

HTTPResponse HTTPConnection::head(const std::string &url) {
    auto dashIdx = url.find('/', 7);
    std::string host = url.substr(7, dashIdx - 7);
    std::string path = dashIdx == std::string::npos ? "/" : url.substr(dashIdx);
    return head(host, path);
}

std::string HTTPConnection::constructHeaders(const std::string &path, const std::string &method) const noexcept {
    std::stringstream ss;
    ss << method << " " << path << " HTTP/1.1\r\n";
    for (const auto &[k, v] : headers) {
        ss << k << ": " << v << "\r\n";
    }
    ss << "\r\n";
    return ss.str();
}

HTTPResponse HTTPConnection::receiveHTTPHeaders() const {
    char response;
    std::string receivedHeaders;
    enum class CharState {
        PLAIN = 0,
        R,
        RN,
        RNR,
        RNRN
    };
    CharState s = CharState::PLAIN;
    while (conn->receive(&response, sizeof(response)) != 0) {
        if (response == '\r') {
            if (s == CharState::RN) {
                s = CharState::RNR;
            } else {
                s = CharState::R;
            }
            // \r字符在接收时直接丢掉
        } else if (response == '\n') {
            if (s == CharState::R) {
                s = CharState::RN;
            } else if (s == CharState::RNR) {
                s = CharState::RNRN;
            } else {
                s = CharState::PLAIN;
            }
            receivedHeaders.push_back(response);
        } else {
            s = CharState::PLAIN;
            receivedHeaders.push_back(response);
        }
        if (s == CharState::RNRN)
            break;
    }
    return HTTPResponse{receivedHeaders};
}

void HTTPConnection::setHeader(const std::string &key, const std::string &val) {
    headers.emplace(key, val);
}

void HTTPConnection::initHeaders() noexcept {
    headers.emplace("Connection", "keep-alive");
    headers.emplace("User-Agent", USER_AGENT);
    headers.emplace("Accept", "*/*");
}

HTTPResponse HTTPConnection::head(const std::string &host, const std::string &fileUrl) {
    if (!conn->connected() && !conn->connect(host)) {
        return HTTPResponse{};
    }

    headers["Host"] = host;
    auto req = constructHeaders(fileUrl, "HEAD");
    conn->send(req.data(), req.length());
    return receiveHTTPHeaders();
}

HTTPResponse HTTPConnection::get(const std::string &host, const std::string &fileUrl) {
    if (!conn->connected() && !conn->connect(host)) {
        return HTTPResponse{};
    }

    headers["Host"] = host;
    auto req = constructHeaders(fileUrl);
    conn->send(req.data(), req.length());

    auto resp = receiveHTTPHeaders();
    // BUF_SIZE = 1MB
    constexpr size_t BUF_SIZE = 1024 * 1024;
    std::vector<char> buf(BUF_SIZE);
    int contentLength = std::stoi(resp["Content-Length"]);

    int len = 0;
    int totalLen = 0;
    std::vector<char> body;
    body.reserve(contentLength);
    while ((len = conn->receive(buf.data(), BUF_SIZE)) != 0) {
        body.insert(body.end(), std::begin(buf), std::begin(buf) + len);
        totalLen += len;
        if (totalLen == contentLength) {
            break;
        }
    }
    resp.parseBody(body);
    return resp;
}

} // namespace multi_get