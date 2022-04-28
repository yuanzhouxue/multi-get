#include "HTTPConnection.h"
#include <cstdio>
#include <cstring>

namespace multi_get {

HTTPResponse HTTPConnection::get(const std::string &url) {
    if (url.substr(0, 7) != "http://") {
        throw "Only support http !!";
    }
    auto dashIdx = url.find('/', 7);
    std::string host = url.substr(7, dashIdx - 7);
    std::string path = dashIdx == std::string::npos ? "/" : url.substr(dashIdx);
    headers["Host"] = host;

    if (socks.find(host) == socks.end() && !connect(host)) {
        return HTTPResponse{};
    }

    auto req = constructHeaders(path);
    ::send(socks[host], req.c_str(), req.length(), 0);

    auto resp = receiveHTTPHeaders(socks[host]);
    // BUF_SIZE = 1MB
    constexpr size_t BUF_SIZE = 1024 * 1024;
    unsigned char buf[BUF_SIZE];
    int contentLength = std::stoi(resp["Content-Length"]);

    int len = 0;
    int totalLen = 0;
    std::vector<char> body;
    body.reserve(contentLength);
    while ((len = ::recv(socks[host], buf, BUF_SIZE, 0)) != 0) {
        body.insert(body.end(), std::begin(buf), std::begin(buf) + len);
        totalLen += len;
        if (totalLen == contentLength) {
            break;
        }
    }
    resp.parseBody(body);
    return resp;
}

HTTPResponse HTTPConnection::head(const std::string &url) {
    if (url.substr(0, 7) != "http://") {
        throw "Only support http !!";
    }
    auto dashIdx = url.find('/', 7);
    std::string host = url.substr(7, dashIdx - 7);
    std::string path = dashIdx == std::string::npos ? "/" : url.substr(dashIdx);
    headers["Host"] = host;

    if (socks.find(host) == socks.end() && !connect(host)) {
        return HTTPResponse{};
    }

    std::vector<char> res;
    auto req = constructHeaders(path, "HEAD");
    ::send(socks[host], req.c_str(), req.length(), 0);
    return receiveHTTPHeaders(socks[host]);
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

HTTPResponse HTTPConnection::receiveHTTPHeaders(int socket) const {
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
    while (::recv(socket, &response, sizeof(response), 0) != 0) {
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
        if (s == CharState::RNRN) break;
    }
    return HTTPResponse{receivedHeaders};
}

bool HTTPConnection::connect(const std::string &host) {
    if (socks.find(host) != socks.end())
        return true;

    int sock = ::socket(PF_INET, SOCK_STREAM, 0);
    auto servIP = ::gethostbyname(host.c_str());

    struct sockaddr_in servAddr;
    std::memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = ::inet_addr(inet_ntoa(*(struct in_addr *)servIP->h_addr_list[0]));
    servAddr.sin_port = ::htons(80);

    auto err = ::connect(sock, (struct sockaddr *)&servAddr, sizeof(servAddr));
    if (err != 0) {
        // std::cout << "connect error: " << err << std::endl;
        ::perror("Connection Error: ");
        // std::cout << "Error no: " << errno << std::endl;
        return false;
    }
    socks[host] = sock;
    return true;
}

void HTTPConnection::setHeader(const std::string &key, const std::string &val) {
    headers.emplace(key, val);
}

void HTTPConnection::initHeaders() noexcept {
    headers.emplace("Connection", "Keep-Alive");
    headers.emplace("User-Agent", "multi-get/0.1.0");
    headers.emplace("Accept", "*/*");
}

HTTPConnection::~HTTPConnection() {
    for (const auto &[host, sock] : socks)
        close(sock);
}

} // namespace multi_get