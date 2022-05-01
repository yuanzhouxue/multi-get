在上一节我们已经分析了HTTP响应的格式如下：

```
[http version] [http status code] [http status code explaination]\r\n
[header key]: [header value]\r\n
[header key]: [header value]\r\n
\r\n
[body]
```

## 接收HTTP响应

我们可以发现，HTTP响应是由起始行，头部，空行，和响应体组成。而在实际接收过程中，我们不知道响应体的具体长度，因此不能直接接收整个HTTP响应，现在的想法是先接收HTTP的头部，然后通过HTTP头部中的Content-Length字段来获取响应体的长度，然后再接收响应体。

```cpp
HTTPResponse receiveHTTPHeaders(int socket) const {
    char response;
    std::string receivedHeaders;
    enum class CharState {
        PLAIN = 0,  // 当前接收到的是普通字符
        R,          // 当前接收到一个\r字符
        RN,         // 当前接收到\r\n
        RNR,        // 接收到\r\n\r
        RNRN        // 接收到\r\n\r\n （这个状态表示头部已经接收完了）
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
```

## 解析HTTP响应

这里写了一个类`HTTPResponse`来表示HTTP响应，其构造函数之一接收一个字符串，这个字符串就是HTTP响应的头部。在这个构造函数里面调用`parse(std::string)`函数来解析HTTP响应的头部。

```cpp
void HTTPResponse::parse(const std::string &resText) {
    ResponseParts parts = ResponseParts::StartLine;
    std::istringstream buffer(resText);
    std::string line;
    // 以行为单位进行解析
    while (std::getline(buffer, line)) {
        switch (parts) {
        // 解析起始行
        case ResponseParts::StartLine: {
            std::istringstream temp(line);
            temp >> this->_httpVersion;
            temp >> this->_status;
            temp.get();
            std::getline(temp, this->_statusText);
            parts = ResponseParts::Headers;
            break;
        }
        // 解析头部
        case ResponseParts::Headers: {
            if (line.length() == 1) {
                parts = ResponseParts::Body;
                break;
            }
            auto idx = line.find(":");
            if (idx == std::string::npos)
                continue;
            int idx2 = idx + 1;
            while (line[idx2] == ' ') ++idx2; // 去除首部空格
            this->_headers[line.substr(0, idx)] = line.substr(idx2);
            break;
        }
        // 解析响应体
        case ResponseParts::Body: {
            this->_body.insert(_body.end(), line.begin(), line.end());
            this->_body.push_back('\n');
            break;
        }
        default:
            break;
        }
    }
}
```cpp

这里，HTTP的头部暂时使用`std::unordered_map<std::string, std::string>`来存储，但需要注意到响应头中可能包含多个key相同的字段，如`Set-Cookie`等，因此这里后续还需要进行优化。

```cpp
using Headers = std::unordered_map<std::string, std::string>;
```
