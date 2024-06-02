
#include <algorithm>
#include <string>
#include <sys/_types/_ssize_t.h>
#include <vector>

#include "Connection.h"
#include "Socket.h"
#include "Logger.h"

namespace multi_get {

union PORT {
    char port_chars[2];
    uint16_t port_short;
};

Connection::Connection(const std::string& hostname, Port port) : hostname(std::move(hostname)), port(port) {
    socket = createSocket(hostname, port);
}

Connection::Connection(const std::string& hostname, Port port, const std::string& proxyStr) : hostname(std::move(hostname)), port(port) {
    std::string p_addr;
    uint16_t p_port = 0;
    do {
        if (proxyStr.substr(0, 9) != "socks5://") break;
        
        bool proxyAddrParsed = false;
        for (int i = 9; i < proxyStr.length(); ++i) {
            if (proxyStr[i] == ':') {
                proxyAddrParsed = true;
                continue;
            }
            if (!proxyAddrParsed) {
                p_addr.push_back(proxyStr[i]);
                continue;
            }
            if (std::isdigit(proxyStr[i])) {
                p_port = p_port * 10 + proxyStr[i] - '0';
            } else {
                LOG_ERROR("Unsupported proxy port.");
                std::cerr << "Unsupported proxy port." << std::endl;
                break;
            }
        }
    } while (0);
    if (p_addr.empty() || p_port == 0) {
        socket = createSocket(hostname, port);
        return;
    }
    proxyAddr = std::move(p_addr);
    proxyPort = p_port;
    socket = createSocket(proxyAddr, proxyPort);
}

Connection::Connection(Connection&& conn) : hostname(std::move(conn.hostname)), port(conn.port), proxyAddr(std::move(conn.proxyAddr)), proxyPort(conn.proxyPort), socket(std::move(conn.socket)) {
    conn.port = conn.proxyPort = 0;
    conn.socket = nullptr;
}
Connection& Connection::operator=(Connection&& conn) {
    hostname = std::move(conn.hostname);
    proxyAddr = std::move(conn.proxyAddr);
    port = conn.port;
    proxyPort = conn.proxyPort;
    socket = std::move(conn.socket);
    conn.port = conn.proxyPort = 0;
    conn.socket = nullptr;
    return *this;
}

bool Connection::doProxyHandshake() const {
    LOG_WARN("Trying proxy socks5://%s:%d", proxyAddr.c_str(), proxyPort);
    char req[3] = {0x05, 0x01, 0x00};
    send(req, sizeof(req));
    char res[2];
    receive(res, 2);

    if (res[0] != 0x05) {
        LOG_ERROR("Proxy server %s:%d is not a socks5 server.", proxyAddr.c_str(), proxyPort);
        std::cerr << "Proxy server is not a socks5 server." << std::endl;
        return false;
    }
    if (res[1] != 0) {
        LOG_ERROR("Proxy server %s:%d doer not support no-auth method.", proxyAddr.c_str(), proxyPort);
        std::cerr << "Proxy server doer not support no-auth method." << std::endl;
        return false;
    }

    std::vector<char> connectReq;
    connectReq.push_back(0x05); // socks version
    connectReq.push_back(0x01); // cmd: connect
    connectReq.push_back(0x00); // RSV
    connectReq.push_back(0x03); // address type: domain name
    connectReq.push_back(static_cast<char>(hostname.length())); // domain length
    connectReq.insert(connectReq.end(), hostname.begin(), hostname.end()); // domain name

    PORT p{0, 0};
    p.port_short = htons(port);
    connectReq.push_back(p.port_chars[0]); // port
    connectReq.push_back(p.port_chars[1]); // port

    send(connectReq.data(), static_cast<int>(connectReq.size()));
    receive(connectReq.data(), static_cast<int>(connectReq.size()));

    constexpr static const char *const SOCKS_ERROR_STR[] = {
        "Succeed",
        "General SOCKS server failure",
        "Connection not allowed by ruleset",
        "Network unreachable",
        "Host unreachable",
        "Connection refused",
        "TTL expired",
        "Command not supported",
        "Address type not supported"
    };
    if (connectReq[1] != 0  && connectReq[1] < sizeof(SOCKS_ERROR_STR) / sizeof(const char* const)) {
        std::cerr << SOCKS_ERROR_STR[connectReq[1]] << std::endl;
        return false;
    }
    return true;
}

void Connection::receiveNBytes(char *_buf, size_t n) const {
    auto remainBytes = n;
    ssize_t len;
    while (remainBytes && (len = receive(_buf, remainBytes)) <= remainBytes) {
        if (len == -1) {
            perror("receive n bytes");
            break;
        } else if (len == 0) {
            std::cerr << "peer closed..." << std::endl;
            break;
        }
        remainBytes -= len;
        _buf += len;
    }
}

bool Connection::connect() {
    if (socket && socket->connect()) {
        return true;
    }
    bool connectSuccess = socket->connect();
    if (!proxyAddr.empty()) {
        connectSuccess = doProxyHandshake();
    }
    return connectSuccess;
}

bool Connection::connected() const noexcept {
    return socket->connected();
}

ssize_t Connection::send(const char *_buf, size_t _n) const {
    return socket->send(_buf, _n);
}

ssize_t Connection::receive(char *_buf, size_t _n) const {
    return socket->receive(_buf, _n);
}

Connection::~Connection() {
    socket->disconnect();
}

} // namespace multi_get
