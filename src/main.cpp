#include <chrono>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "HTTPConnection.h"

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

using namespace std;

namespace multi_get {

std::mutex m;

size_t downloadRange(const string &url, ssize_t beginPos, ssize_t endPos, const std::string &proxy = "") {
    multi_get::HTTPConnection conn{};
    if (!proxy.empty())
        conn.setProxy(proxy);

    string filename = url.substr(url.find_last_of('/') + 1);
    if (filename.empty())
        filename = "multi-get.downloaded";

    if (beginPos >= 0 && endPos >= beginPos) {
        std::stringstream ss;
        ss << "bytes=" << beginPos << '-' << endPos;
        conn.setHeader("Range", ss.str());
        ss.str("");
        ss << '.' << beginPos << '-' << endPos;
        filename.append(ss.str());
    }
    auto res = conn.get(url);
    ofstream out(filename, ios::binary);
    out.write(res.body().data(), res.body().size());
    out.close();

    lock_guard<std::mutex> locker(m);
    std::cout << "Thread " << this_thread::get_id() << " downloaded " << res.contentLength() << " bytes" << std::endl;
//    cout << beginPos << " " << endPos << endl;
    return res.contentLength();
}

size_t download(const string &url, size_t threadCount = 1, const std::string &proxy = "") {
    if (threadCount < 1)
        threadCount = 1;
    if (threadCount > 32)
        threadCount = 32;

    cout << "Downloading using " << threadCount << " threads..." << endl;
    auto start = std::chrono::system_clock::now();

    multi_get::HTTPConnection conn{};
    if (!proxy.empty())
        conn.setProxy(proxy);
    auto res = conn.head(url);

    //    res.displayHeaders();

    if (!res.contains("Content-Length") || (threadCount > 1 && res["Accept-Ranges"] != string("bytes"))) {
        std::cout << "The server does not support range request, using single thread to download!" << std::endl;
        return downloadRange(url, -1, -1);
    }

    ssize_t fileSize = std::stoi(res["Content-Length"]);

    /* 必须注意到Ranges两端都是闭区间 */
    ssize_t perThreadSize = fileSize / threadCount;
    ssize_t remain = fileSize % threadCount;
    ssize_t idx = -1;

    vector<std::thread> threads(threadCount);
    vector<pair<size_t, size_t>> ranges;
    for (int i = 0; i < threadCount; ++i) {
        auto range = perThreadSize;
        if (remain-- > 0)
            ++range;
        // cout << "Thread ranges: " << idx + 1<< "-" << idx + range << endl;
        ranges.emplace_back(idx + 1, idx + range);
        threads[i] = std::thread{downloadRange, url, idx + 1, idx + range, proxy};
        idx += range;
    }

    for (int i = 0; i < threadCount; ++i) {
        threads[i].join();
    }

    string filename = url.substr(url.find_last_of('/') + 1);
    if (filename.empty())
        filename = "multi-get.downloaded";

    if (filesystem::exists(filename))
        filesystem::remove(filename);

    ofstream output;
    stringstream ss;
    ss << filename << '.' << ranges[0].first << '-' << ranges[0].second;
    string tempName = ss.str();
    output.open(tempName, std::ios::binary | std::ios::out | std::ios::app);
    for (int i = 1; i < ranges.size(); ++i) {
        ss.str("");
        ss << filename << '.' << ranges[i].first << '-' << ranges[i].second;
        ifstream input;
        input.open(ss.str(), std::ios::binary | std::ios::in);
        output << input.rdbuf();
        input.close();
        filesystem::remove(ss.str());
    }
    output.close();
    // just need rename
    filesystem::rename(tempName, filename);

    auto end = std::chrono::system_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    auto secondsUsed = double(duration.count()) * chrono::microseconds::period::num / chrono::microseconds::period::den;
    cout << "Time spent: " << secondsUsed << "s" << endl;
    auto KBps = fileSize / secondsUsed / 1024.0;
    cout << "Average speed: ";
    if (KBps < 1024) {
        cout << KBps << " KB/s" << endl;
    } else if (KBps < 1024 * 1024) {
        cout << KBps / 1024.0 << " MB/s" << endl;
    } else if (KBps < 1024 * 1024 * 1024) {
        cout << KBps / 1024.0 / 1024.0 << " GB/s" << endl;
    } else {
        cout << KBps / 1024.0 / 1024.0 / 1024.0 << " TB/s" << endl;
    }

    return fileSize;
}

} // namespace multi_get

void showUsage() {
    cout << "Usage: multi-get [-n N] [-x proxy] <url>" << endl;
    cout << "  -n N:        download using N threads, default is 4" << endl;
    cout << "  -x proxy:    download using proxy, only support socks5 proxy now." << endl;
    cout << "  -h:          show this help" << endl;
    cout << "example:" << endl;
    cout << "multi-get https://example.com" << endl;
    cout << "multi-get https://example.com -n 16" << endl;
    cout << "multi-get https://example.com -n 16 -x socks5://localhost:1080" << endl;
}

class CmdParser {
    unordered_map<string, string> keywords;
    vector<string> positional;
    string executableName;

  public:
    void clear() {
        positional.clear();
        keywords.clear();
        executableName.clear();
    }

    CmdParser(int argc, const char **argv) {
        clear();
        if (argc >= 1) {
            executableName = string(argv[0]);
            for (int i = 1; i < argc;) {
                if (argv[i][0] == '-') {
                    if (i >= argc || argv[i + 1][0] == '-') {
                        keywords.emplace(string(argv[i]), "");
                        ++i;
                    } else {
                        keywords.emplace(string(argv[i]), string(argv[i + 1]));
                        i += 2;
                    }
                } else {
                    positional.emplace_back(argv[i]);
                    ++i;
                }
            }
        }
    }

    string get(const string &key, const string &defaultValue = "") const {
        if (keywords.count(key))
            return keywords.find(key)->second;
        return defaultValue;
    }

    string get(size_t idx, const string &defaultValue = "") const {
        if (idx >= positional.size())
            return defaultValue;
        return positional.at(idx);
    }

    bool contains(const string& key) const {
        return keywords.count(key);
    }

    bool contains(const vector<string>& list) {
        for (const auto& l : list) {
            if (keywords.count(l)) return true;
        }
        return false;
    }

    size_t numPositionalArgs() const noexcept {
        return positional.size();
    }
};

int main(int argc, const char **argv) {

#ifdef _WIN32
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA data;
    if (WSAStartup(sockVersion, &data) != 0) {
        return 1;
    }
#endif //_WIN32

    CmdParser parser{argc, argv};
    if (parser.numPositionalArgs() != 1 || parser.contains({"-h", "--help"})) {
        showUsage();
        return 0;
    }

    std::string url = parser.get(0);
    int threadCount = 4;
    std::string proxy;

    if (parser.contains("-n")) {
        threadCount = 0;
        for (const char c : parser.get("-n")) {
            if (!isdigit(c)) {
                cerr << "Invalid thread count. Using thread count = 4!" << endl;
                threadCount = 4;
                break;
            }
            threadCount = threadCount * 10 + c - '0';
        }
    }
    proxy = parser.get("-x", "");



    multi_get::download(url, threadCount, proxy);

#ifdef _WIN32
    WSACleanup();
#endif // _WIN32

    return 0;
}
