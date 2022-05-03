## HTTPS原理

HTTPS通信是在HTTP通信的基础上加入SSL/TLS的加密协议，它是一种安全的通信协议，可以保证数据的安全传输。示意图如下：

```
HTTP:   TCP <<==>> HTTP
HTTPS:  TCP <<==>> SSL <<==>> HTTP
```

因此，我们可以在不改动HTTP协议的基础上，添加SSL/TLS的加密协议，实现HTTPS通信。

## 基于OpenSSL的HTTPS实现

主要分为以下几个步骤：

- 初始化OpenSSL库
- 创建CTX
- 创建SSL套接字
- 完成SSL握手

代码如下：

```c++
// 先建立Socket连接
if (!Connection::connect(host)) {
    return false;
}

// 初始化OpenSSL库（不同的OpenSSL版本有不同的初始化方式）
#if OPENSSL_VERSION_NUMBER >= 0x10100003L
    if (OPENSSL_init_ssl(OPENSSL_INIT_LOAD_CONFIG, NULL) == 0) {
        std::cerr << "Error creating SSL connection." << std::endl;
        _connected = false;
        return false;
    }
#else
    OPENSSL_config(NULL) l
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
#endif

// 创建CTX
const SSL_METHOD *m = TLS_client_method(); // TLS
SSL_CTX *ctx = SSL_CTX_new(m);

// 创建SSL套接字
ssl = SSL_new(ctx);
if (!ssl) {
    std::cerr << "Error creating SSL connection." << std::endl;
    _connected = false;
    return false;
}
SSL_set_fd(ssl, sock); // 将SSL与底层Socket绑定

// 完成SSL握手
int err = SSL_connect(ssl);
if (err <= 0) {
    auto error = SSL_get_error(ssl, err);
    std::cerr << "Error creating SSL connection: " << err << std::endl;
    _connected = false;
    return false;
}
```

建立SSL连接后，只需要将上层发送和接收的数据通过SSL套接字进行包装即可。

```c++
// 发送数据的函数
ssize_t SSLConnection::receive(void *__buf, size_t __n) const {
    int len = SSL_read(ssl, __buf, __n);
    if (len < 0) {
        int err = SSL_get_error(ssl, len);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            return 0;
        } else if (err == SSL_ERROR_ZERO_RETURN || err == SSL_ERROR_SYSCALL || err == SSL_ERROR_SSL) {
            return -1;
        }
    }
    return len;
}

// 接收数据的函数
ssize_t SSLConnection::send(const void *__buf, size_t __n) const {
    int len = SSL_write(ssl, __buf, __n);
    if (len < 0) {
        int err = SSL_get_error(ssl, len);
        if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ) {
            return 0;
        } else {
            return -1;
        }
    }
    return len;
}
```

与之对比，在HTTP中不需要进行包装，而直接通过Socket发送即可。

```c++
// 发送数据的函数
ssize_t send(const void *__buf, size_t __n) const {
    return ::send(sock, __buf, __n, 0);
}
// 接收数据的函数
ssize_t receive(void *__buf, size_t __n) const {
    return ::recv(sock, __buf, __n, 0);
}
```
