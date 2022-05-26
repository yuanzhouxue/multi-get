//
// Created by xyz on 22-5-22.
//

#ifndef MULTI_GET_POOL_H
#define MULTI_GET_POOL_H

#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include "Connection.h"

namespace multi_get {

class Pool {
  private:
    std::unordered_map<std::string, std::queue<std::shared_ptr<Connection>>> http_pool;
    //    std::unordered_map<std::string, std::queue<std::shared_ptr<Connection>>> https_pool;
    std::mutex m;

    Pool() {
        LOG_INFO("Initializing connection pool...");
    }

    static std::shared_ptr<Connection> createConnection(const std::string &protocol, const std::string &hostname, uint16_t port, const std::string& proxy) {
        std::shared_ptr<Connection> conn;
        if (protocol == "https")
            conn = std::make_shared<SSLConnection>(hostname, port, proxy);
        else
            conn = std::make_shared<PlainConnection>(hostname, port, proxy);

        int retry = 5;
        while (retry--) {
            if (conn->connect())
                return conn;
        }
        LOG_ERROR("Connection to %s failed.", hostname.c_str());
        return conn;
    }
    ~Pool() {
        size_t size = 0;
        for (const auto& [host, pool] : http_pool) {
            size += pool.size();
        }
        LOG_INFO("Pool size: %ld", size);
    }

  public:
    std::shared_ptr<Connection> get(const std::string &url, const std::string& proxy = "") {
        const auto [protocol, hostname, port] = formatHost(url);
        std::stringstream ss;
        ss << protocol << "://" << hostname << ':' << port;
        const std::string key = ss.str();
        if (http_pool.count(key) && !http_pool[key].empty()) {
            auto &pool = http_pool[key];
            std::unique_lock<std::mutex> locker(m);
            auto res = std::move(pool.front());
            pool.pop();
            locker.unlock();
            return res;
        } else {
            // need to create connection
            return createConnection(protocol, hostname, port, proxy);
        }
    }

    void put(const std::string &url, std::shared_ptr<Connection> &&conn) {
        const auto [protocol, hostname, port] = formatHost(url);
        std::stringstream ss;
        ss << protocol << "://" << hostname << ':' << port;
        const std::string key = ss.str();

        http_pool[key].push(std::move(conn));
    }

    static Pool &getInstance() {
        static Pool pool;
        return pool;
    }
};

class PoolGuard {
  private:
    std::shared_ptr<Connection> conn;
    std::string url;
  public:
    [[nodiscard]] const std::shared_ptr<Connection>& get() const {
        return conn;
    }

    void release() {
        Pool::getInstance().put(url, move(conn));
        conn.reset();
    }

    explicit PoolGuard(const std::string& url, const std::string& proxy = "") : url(url) {
        conn = Pool::getInstance().get(url, proxy);
    }

    ~PoolGuard() {
        if (conn) release();
    }

    const std::shared_ptr<Connection>& operator->() const noexcept {
        return conn;
    }
};

} // namespace multi_get

#endif // MULTI_GET_POOL_H
