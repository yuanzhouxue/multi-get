#include "HTTPResponse.h"

namespace multi_get {

void HTTPResponse::displayHeaders() const noexcept {
    std::cout << _httpVersion << " " <<  _status << " " << _statusText << std::endl;
    for (const auto &[k, v] : _headers) {
        std::cout << k << ": " << v << std::endl;
    }
}

void HTTPResponse::parse(const std::string &resText) {
    // for (char c : resText) std::cout << c;
    // std::cout << std::endl;
    ResponseParts parts = ResponseParts::StartLine;
    std::istringstream buffer(resText);
    std::string line;
    while (std::getline(buffer, line)) {
        switch (parts) {
        case ResponseParts::StartLine: {
            std::istringstream temp(line);
            temp >> this->_httpVersion;
            temp >> this->_status;
            temp.get();
            std::getline(temp, this->_statusText);
            parts = ResponseParts::Headers;
            break;
        }
        case ResponseParts::Headers: {
            if (line.length() == 1) {
                parts = ResponseParts::Body;
                break;
            }
            auto idx = line.find(':');
            if (idx == std::string::npos)
                continue;
            int idx2 = idx + 1;
            while (line[idx2] == ' ') ++idx2; // 去除首部空格
            this->_headers[line.substr(0, idx)] = line.substr(idx2); // 去除尾部\r
            break;
        }
        case ResponseParts::Body: {
            this->_body.insert(_body.end(), line.begin(), line.end());
            this->_body.push_back('\n');
            break;
        }
        }
    }
}

void HTTPResponse::parse(const std::vector<char> &data) {
    // for (char c : data) std::cout << c;
    // std::cout << std::endl;
    ResponseParts parts = ResponseParts::StartLine;
    std::string line;
    size_t pos = 0;
    bool parsed = false;
    while (!parsed) {
        getline(data, pos, line);
        switch (parts) {
        case ResponseParts::StartLine: {
            std::istringstream temp(line);
            temp >> this->_httpVersion;
            temp >> this->_status;
            temp.get();
            std::getline(temp, this->_statusText);
            parts = ResponseParts::Headers;
            break;
        }
        case ResponseParts::Headers: {
            if (line.length() == 1) {
                parts = ResponseParts::Body;
                break;
            }
            auto idx = line.find(':');
            if (idx == std::string::npos)
                continue;
            auto idx2 = idx + 1;
            while (line[idx2] == ' ') ++idx2; // 去除首部空格
            this->_headers[line.substr(0, idx)] = line.substr(idx2);
            break;
        }
        case ResponseParts::Body: {
            parsed = true;
            break;
        }
        default:
            break;
        }
    }
}

bool HTTPResponse::getline(const std::vector<char> &data, size_t &pos, std::string &val) {
    val.clear();
    while (data[pos] != '\n') {
        val.push_back(data[pos]);
        ++pos;
    }
    ++pos;
    return true;
}

} // namespace multi_get