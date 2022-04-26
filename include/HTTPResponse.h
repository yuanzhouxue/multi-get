#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <unordered_map>
#include <iostream>
#include <vector>
#include <sstream>


namespace multi_get {

using Headers = std::unordered_map<std::string, std::string>;

class HTTPResponse {
  private:
    Headers _headers;
    size_t _contentLength{0};
    enum class ResponseParts {
        StartLine = 0,
        Headers,
        Body
    };
    std::string _res;
    int _status{-1};
    std::string _statusText;
    std::vector<char> _body;
    std::string _httpVersion;

    void parse(const std::string &resText);

    void parse(const std::vector<char> &data);

    static bool getline(const std::vector<char> &data, size_t &pos, std::string &val);

  public:
    explicit HTTPResponse(const std::string &text) {
        parse(text);
    }
    explicit HTTPResponse(const std::vector<char> &buffer) {
        parse(buffer);
    }

    explicit HTTPResponse() = default;

    void parseHeaders(const std::string &h) {
        parse(h);
    }

    void parseBody(std::vector<char> &b) {
        _body = std::move(b);
        _contentLength = _body.size();
    }

    std::string operator[](const std::string &key) const noexcept {
        if (_headers.count(key))
            return _headers.at(key);
        return "";
    }

    const char *data() const noexcept {
        return _body.data();
    }

    const auto contentLength() const noexcept {
        return _body.size();
    }

    const auto &body() const noexcept {
        return _body;
    }

    const auto &status() const noexcept { return this->_status; }

    const auto &statusText() const noexcept {
        return _statusText;
    }

    const auto &httpVersion() const noexcept {
        return this->_httpVersion;
    }

    void displayHeaders() const noexcept;
};
} // namespace std

#endif