#include <cerrno>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <chrono>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

using Headers = unordered_map<string, string>;

static void handle_term(int sig) {
}

namespace multi_get {
class HTTPResponse {
  private:
    Headers _headers;
    size_t _contentLength{0};
    enum ResponseParts {
        StartLine = 0,
        Headers,
        Body
    };
    string _res;
    int _status{-1};
    string _statusText;
    vector<char> _body;
    string _httpVersion;

    void parse(const string &resText) {
        ResponseParts parts = StartLine;
        std::istringstream buffer(resText);
        string line;
        while (std::getline(buffer, line)) {
            switch (parts) {
            case StartLine: {
                istringstream temp(line);
                temp >> this->_httpVersion;
                temp >> this->_status;
                temp.get();
                std::getline(temp, this->_statusText);
                parts = Headers;
                break;
            }
            case Headers: {
                if (line.length() == 1) {
                    parts = Body;
                    break;
                }
                auto idx = line.find(":");
                if (idx == std::string::npos)
                    continue;
                this->_headers[line.substr(0, idx)] = line.substr(idx + 2);
                break;
            }
            case Body: {
                this->_body.insert(_body.end(), line.begin(), line.end());
                this->_body.push_back('\n');
                break;
            }
            default:
                break;
            }
        }
    }

    void parse(const vector<char> &data) {
        ResponseParts parts = StartLine;
        string line;
        size_t pos = 0;
        bool parsed = false;
        while (!parsed) {
            getline(data, pos, line);
            switch (parts) {
            case StartLine: {
                istringstream temp(line);
                temp >> this->_httpVersion;
                temp >> this->_status;
                temp.get();
                std::getline(temp, this->_statusText);
                parts = Headers;
                break;
            }
            case Headers: {
                if (line.length() == 1) {
                    parts = Body;
                    break;
                }
                auto idx = line.find(":");
                if (idx == std::string::npos)
                    continue;
                this->_headers[line.substr(0, idx)] = line.substr(idx + 2);
                break;
            }
            case Body: {
                this->_contentLength = data.size() - pos;
                parsed = true;
                break;
            }
            default:
                break;
            }
        }
    }

    bool getline(const vector<char> &data, size_t &pos, string &val) {
        val.clear();
        while (data[pos] != '\n') {
            val.push_back(data[pos]);
            ++pos;
        }
        ++pos;
        return true;
    }

  public:
    explicit HTTPResponse(const string &text) {
        parse(text);
    }
    explicit HTTPResponse(const vector<char> &buffer) {
        parse(buffer);
    }

    explicit HTTPResponse() = default;

    void parseHeaders(const string& h) {
        parse(h);
    }

    void parseBody(vector<char>& b) {
        _body = std::move(b);
        _contentLength = _body.size();
    }

    string operator[](const string &key) const noexcept {
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

    void displayHeaders() const noexcept {
        for (const auto &[k, v] : _headers) {
            cout << k << ": " << v << endl;
        }
    }
};

class HTTPConnection {
  public:
    HTTPConnection() {
        initHeaders();
    }
    // HTTPConnection(const string &url) {
    //     if (url.substr(0, 7) != "http://") {
    //         throw "Only support http !!";
    //     }
    //     auto dashIdx = url.find('/', 7);
    //     host = url.substr(7, dashIdx - 7);
    //     path = dashIdx == string::npos ? "/" : url.substr(dashIdx);
    //     // cout << host << " " << path << endl;
    //     headers.emplace("Host", host);
    //     initHeaders();

    //     sock = socket(PF_INET, SOCK_STREAM, 0);

    //     auto servIP = gethostbyname(host.c_str());
    //     struct sockaddr_in servAddr;
    //     memset(&servAddr, 0, sizeof(servAddr));

    //     servAddr.sin_family = AF_INET;
    //     servAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr *)servIP->h_addr_list[0]));
    //     servAddr.sin_port = htons(80);

    //     auto err = ::connect(sock, (struct sockaddr *)&servAddr, sizeof(servAddr));
    //     if (err != 0) {
    //         std::cout << "connect error: " << err << std::endl;
    //         exit(-1);
    //     }
    // }

    HTTPResponse get(const string &url) {
        if (url.substr(0, 7) != "http://") {
            throw "Only support http !!";
        }
        auto dashIdx = url.find('/', 7);
        string host = url.substr(7, dashIdx - 7);
        string path = dashIdx == string::npos ? "/" : url.substr(dashIdx);
        headers["Host"] = host;

        if (socks.find(host) == socks.end() && !connect(host)) {
            return HTTPResponse{};
        }

        vector<char> res;
        auto req = constructHeaders(path);
        ::send(socks[host], req.c_str(), req.length(), 0);

        char response;
        bool gotHeaders = false;
        string receivedHeaders;
        while (::recv(socks[host], &response, sizeof(response), 0) != 0) {
            receivedHeaders.push_back(response);
            if (response == '\r') {
                if (::recv(socks[host], &response, sizeof(response), 0) != 0) {
                    receivedHeaders.push_back(response);
                    if (response == '\n') {
                        if (::recv(socks[host], &response, sizeof(response), 0) != 0) {
                            receivedHeaders.push_back(response);
                            if (response == '\r') {
                                if (::recv(socks[host], &response, sizeof(response), 0) != 0) {
                                    receivedHeaders.push_back(response);
                                    if (response == '\n') {
                                        gotHeaders = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        HTTPResponse resp(receivedHeaders);
        // BUF_SIZE = 1MB
        constexpr size_t BUF_SIZE = 1024 * 1024;
        if (gotHeaders == true) {
            int contentLength = stoi(resp["Content-Length"]);
            unsigned char buf[BUF_SIZE];
            int len = 0;
            int totalLen = 0;
            vector<char> body;
            body.reserve(contentLength);
            while ((len = recv(socks[host], buf, BUF_SIZE, 0)) != 0) {
                body.insert(body.end(), begin(buf), begin(buf) + len);
                totalLen += len;
                if (totalLen == stoi(resp["Content-Length"])) {
                    break;
                }
            }
            resp.parseBody(body);
        }
        return resp;
    }

    string constructHeaders(const string &path) {
        stringstream ss;
        ss << "GET " << path << " HTTP/1.1\r\n";
        for (const auto &[k, v] : headers) {
            ss << k << ": " << v << "\r\n";
        }
        ss << "\r\n";
        return ss.str();
    }

    void initHeaders() {
        headers.emplace("Connection", "Keep-Alive");
        headers.emplace("User-Agent", "multi-get/0.1.0");
        headers.emplace("Accept", "*/*");
    }

    void setHeader(const string &key, const string &val) {
        headers.emplace(key, val);
    }

    virtual ~HTTPConnection() {
        for (const auto &[host, sock] : socks)
            close(sock);
    }

  private:
    std::vector<char> res;
    string req;
    Headers headers;

    unordered_map<string, int> socks;

    bool connect(const string &host) {
        if (socks.find(host) != socks.end()) return true;

        int sock = socket(PF_INET, SOCK_STREAM, 0);
        auto servIP = gethostbyname(host.c_str());

        struct sockaddr_in servAddr;
        memset(&servAddr, 0, sizeof(servAddr));
        servAddr.sin_family = AF_INET;
        servAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr *)servIP->h_addr_list[0]));
        servAddr.sin_port = htons(80);

        auto err = ::connect(sock, (struct sockaddr *)&servAddr, sizeof(servAddr));
        if (err != 0) {
            std::cout << "connect error: " << err << std::endl;
            cout << "Error no: " << errno << endl;
            return false;
        }
        socks[host] = sock;
        return true;
    }
};
} // namespace multi_get

int main(int, char **) {
    signal(SIGTERM, handle_term);

    auto conn = multi_get::HTTPConnection();

    // string url = "http://mirrors.tuna.tsinghua.edu.cn/ubuntu/pool/main/t/tcpdump/tcpdump_4.99.1.orig.tar.gz";
    string url = "http://mirrors.tuna.tsinghua.edu.cn/index.html";

    auto start = std::chrono::system_clock::now();
    auto res = conn.get(url);
    auto end = std::chrono::system_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    
    
    res.displayHeaders();

    auto secondsUsed = double(duration.count()) * chrono::microseconds::period::num / chrono::microseconds::period::den;
    cout << "花费了" << secondsUsed << "s" << endl;
    cout << "平均下载速度：" << res.body().size() / secondsUsed / 1024 << " kB/s" << endl;


    // // cout << res.httpVersion() << endl;
    // // cout << res.status() << endl;
    // // cout << res.statusText() << endl;
    // cout << "Body length: " << res.body().size() << endl;
    // cout << res.contentLength() << endl;


    string filename = url.substr(url.find_last_of('/') + 1);
    if (filename.empty()) filename = "multi-get.downloaded";

    // cout << filename << endl;
    ofstream out(filename);
    out.write(res.body().data(), res.body().size());
    out.close();

    cout << res.contentLength() << endl;

    // string resBody{res.body().begin(), res.body().end()};
    // cout << resBody << endl;
}
