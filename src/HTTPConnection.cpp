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
    std::cout << "Heading url: " << url << std::endl;

    auto conn = PoolGuard(url, proxy);

    if (!conn->connected() && !conn->connect()) {
        return HTTPResponse{};
    }
    auto [_, hostname, _port] = formatHost(url);
    headers["Host"] = hostname;
    auto req = constructHeaders(url, "HEAD");
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
    std::cerr << "Getting url: " << url << std::endl;
    auto conn = PoolGuard(url, proxy);

    if (!conn->connected() && !conn->connect()) {
        return HTTPResponse{};
    }

    auto [_, hostname, _port] = formatHost(url);
    headers["Host"] = hostname;
    auto req = constructHeaders(url);
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
        int contentLength = std::stoi(resp["Content-Length"]);
        ssize_t len = 0;
        size_t totalLen = 0;
        body.reserve(contentLength);
        while ((len = conn->receive(buf.data(), buf.size())) != 0) {
            body.insert(body.end(), std::begin(buf), std::begin(buf) + len);
            totalLen += len;
            if (totalLen == contentLength) {
                break;
            }
        }
    } else if (resp.contains("Transfer-Encoding") && resp["Transfer-Encoding"].find("chunked") != std::string::npos) {
        // 采用chunk方式传输
        //        std::cerr << "chunked" << std::endl;
        char response;
        size_t chunkLen = 0;

        ssize_t len = 0;
        while (true) {
            chunkLen = 0;
            while (conn->receive(&response, 1) != 0) {
                if (response == '\r') {
                    conn->receive(&response, 1);
                    break;
                } else {
                    if (response >= '0' && response <= '9')
                        chunkLen = chunkLen * 16 + response - '0';
                    else if (tolower(response) >= 'a' && tolower(response) <= 'f') {
                        chunkLen = chunkLen * 16 + 10 + tolower(response) - 'a';
                    }
                }
            }
            if (chunkLen == 0)
                break;

            if (buf.size() < chunkLen) {
                try {
                    buf.resize(chunkLen);
                } catch (std::exception &e) {
                    std::cerr << "Failed to alloc memory, download failed." << std::endl;
                    std::cerr << e.what() << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            auto remainBytes = chunkLen;
            while (remainBytes && (len = conn->receive(buf.data(), remainBytes)) <= remainBytes) {
                if (len == -1) {
                    if (errno == EWOULDBLOCK || errno == EAGAIN) {
                        std::cerr << "recv timeout ...\n";
                        break;
                    } else if (errno == EINTR) {
                        std::cerr << "interrupt by signal..." << std::endl;
                        continue;
                    } else if (errno == ENOENT) {
                        std::cerr << "recv RST segement..." << std::endl;
                        break;
                    } else {
                        std::cerr << "unknown error!" << std::endl;
                        exit(1);
                    }
                } else if (len == 0) {
                    std::cerr << "peer closed..." << std::endl;
                    break;
                } else {
                    body.insert(body.end(), std::begin(buf), std::begin(buf) + len);
                    //                if (len != buf.size())
                    //                    break;
                }
                remainBytes -= len;
            }

            //  取出\r\n
            conn->receive(&response, 1);
            conn->receive(&response, 1);
        }
    } else {
        ssize_t len = 0;
        while (true) {
            len = conn->receive(buf.data(), buf.size());
            if (len == -1) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    std::cerr << "recv timeout ...\n";
                    break;
                } else if (errno == EINTR) {
                    std::cerr << "interrupt by signal..." << std::endl;
                    continue;
                } else if (errno == ENOENT) {
                    std::cerr << "recv RST segement..." << std::endl;
                    break;
                } else {
                    std::cerr << "unknown error!" << std::endl;
                    exit(1);
                }
            } else if (len == 0) {
                std::cerr << "peer closed..." << std::endl;
                break;
            } else {
                body.insert(body.end(), std::begin(buf), std::begin(buf) + len);
                //                if (len != buf.size())
                //                    break;
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