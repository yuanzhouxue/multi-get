#include "HTTPConnection.h"
#include "version.h"
#include <cstring>

namespace multi_get {

std::string HTTPConnection::constructHeaders(const std::string &path, const std::string &method) const noexcept {
    std::stringstream ss;
    ss << method << " " << path << " HTTP/1.1\r\n";
    for (const auto &[k, v] : headers) {
        ss << k << ": " << v << "\r\n";
    }
    ss << "\r\n";
    return ss.str();
}

HTTPResponse HTTPConnection::receiveHTTPHeaders(const std::shared_ptr<Connection> &conn) {
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

HTTPResponse HTTPConnection::head(const std::string &url) {
    LOG_INFO("Heading url: %s", url.c_str());

    auto conn = PoolGuard(url, proxy);

    if (!conn->connected() && !conn->connect()) {
        return HTTPResponse{};
    }
    auto [_, hostname, _port, path] = formatHost(url);
    headers["Host"] = hostname;
    auto req = constructHeaders(path, "HEAD");
    conn->send(req.data(), req.length());
    auto res = receiveHTTPHeaders(conn);
    //    res.displayHeaders();
    if (res.status() == 301 || res.status() == 302) {
        conn.release();
        res = head(res["Location"]);
    }
    return res;
}

HTTPResponse HTTPConnection::get(const std::string &url) {
    LOG_INFO("Getting url: %s", url.c_str());
    auto conn = PoolGuard(url, proxy);

    if (!conn->connected() && !conn->connect()) {
        return HTTPResponse{};
    }

    auto [_, hostname, _port, path] = formatHost(url);
    headers["Host"] = hostname;
    auto req = constructHeaders(path);
    conn->send(req.data(), req.length());

    auto resp = receiveHTTPHeaders(conn);
    //        resp.displayHeaders();
    if (resp.status() == 301 || resp.status() == 302) {
        return get(resp["Location"]);
    }
    // BUF_SIZE = 1MB
    constexpr size_t BUF_SIZE = 1024 * 1024;
    std::vector<char> buf(BUF_SIZE);
    std::vector<char> body;
    if (resp.contains("Content-Length")) {
        auto contentLength = std::stoull(resp["Content-Length"]);
        body.resize(contentLength);
        conn->receiveNBytes(body.data(), contentLength);
    } else if (resp.contains("Transfer-Encoding") && resp["Transfer-Encoding"].find("chunked") != std::string::npos) {
        char response;
        size_t chunkLen = 0;
        std::string chunkLength;
        while (true) {
            while (conn->receive(&response, 1) != 0) {
                if (response == '\r') {
                    conn->receive(&response, 1);
                    break;
                } else {
                    chunkLength += response;
                }
            }
            chunkLen = std::stoull(chunkLength, nullptr, 16);
            chunkLength.clear();
            if (chunkLen == 0)
                break;

            if (buf.size() < chunkLen) {
                try {
                    buf.resize(chunkLen);
                } catch (std::exception &e) {
                    LOG_INFO("Failed to alloc memory, download failed.");
                    std::cerr << "Failed to alloc memory, download failed." << std::endl;
                    std::cerr << e.what() << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            conn->receiveNBytes(buf.data(), chunkLen);
            body.insert(end(body), begin(buf), begin(buf) + chunkLen);
            //  取出\r\n
            conn->receiveNBytes(buf.data(), 2);
        }
    } else {
        ssize_t len = 0;
        while (true) {
            len = conn->receive(buf.data(), buf.size());
            if (len < 0) {
                perror("receive without length");
                break;
            } else if (len == 0) {
                std::cerr << "peer closed..." << std::endl;
                break;
            } else {
                body.insert(body.end(), std::begin(buf), std::begin(buf) + len);
            }
        }
    }
    resp.parseBody(body);
    return resp;
}
void HTTPConnection::setProxy(const std::string &_proxy) {
    this->proxy = _proxy;
}

} // namespace multi_get