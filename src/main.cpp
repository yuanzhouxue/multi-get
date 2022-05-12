#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>

#include "HTTPResponse.h"
#include "HTTPSConnection.h"

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

using namespace std;

namespace multi_get {

std::mutex m;

void downloadRange(const string &url, size_t beginPos, size_t endPos) {
    multi_get::HTTPSConnection conn{};
    std::stringstream ss;
    ss << "bytes=" << beginPos << '-' << endPos;
    conn.setHeader("Range", ss.str());
    auto res = conn.get(url);
    // res.displayHeaders();

    string filename = url.substr(url.find_last_of('/') + 1);
    if (filename.empty())
        filename = "multi-get.downloaded";

    ss.str("");
    ss << '.' << beginPos << '-' << endPos;
    filename.append(ss.str());

    ofstream out(filename);
    out.write(res.body().data(), res.body().size());
    out.close();

    lock_guard<std::mutex> locker(m);
    std::cout << "Thread " << this_thread::get_id() << " downloaded " << res.contentLength() << " bytes" << std::endl;
}

int download(const string &url, size_t threadCount = 1) {
    if (threadCount < 1)
        threadCount = 1;
    if (threadCount > 32)
        threadCount = 32;

    multi_get::HTTPSConnection conn{};
    auto res = conn.head(url);

    res.displayHeaders();

    if (threadCount > 1 && res["Accept-Ranges"] != string("bytes")) {
        std::cout << "The server does not support range request, using single thread to download!" << std::endl;
        threadCount = 1;
    }

    int fileSize = std::stoi(res["Content-Length"]);

    /* 必须注意到Ranges两端都是闭区间 */
    int perThreadSize = fileSize / threadCount;
    int remain = fileSize % threadCount;
    int idx = -1;

    vector<std::thread> threads(threadCount);
    vector<pair<size_t, size_t>> ranges;
    for (int i = 0; i < threadCount; ++i) {
        int range = perThreadSize;
        if (remain-- > 0)
            ++range;
        // cout << "Thread ranges: " << idx + 1<< "-" << idx + range << endl;
        ranges.emplace_back(idx + 1, idx + range);
        threads[i] = std::thread{downloadRange, url, idx + 1, idx + range};
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
    output.open(ss.str(), std::ios::binary | std::ios::out | std::ios::app);
    for (int i = 1; i < ranges.size(); ++i) {
        stringstream ss;
        ss << filename << '.' << ranges[i].first << '-' << ranges[i].second;
        ifstream input;
        input.open(ss.str(), std::ios::binary | std::ios::in);
        output << input.rdbuf();
        input.close();
        filesystem::remove(ss.str());
    }
    output.close();
    // just need rename
    filesystem::rename(ss.str(), filename);
    return fileSize;
}

} // namespace multi_get

void showUsage() {
    cout << "Usage: multi-get [-n N] <url>" << endl;
    cout << "  -n N: download using N threads, default is 4" << endl;
    cout << "  -h, --help: show this help" << endl;
}


int main(int argc, char ** argv) {

#ifdef _WIN32
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA data;
    if (WSAStartup(sockVersion, &data) != 0) {
        return 1;
    }
#endif //_WIN32

    if (argc == 2 && (string(argv[1]) == "-h" || string(argv[1]) == "--help")) {
        showUsage();
        return 0;
    }

    std::string url = "https://mirrors.tuna.tsinghua.edu.cn/ubuntu/pool/main/t/tcpdump/tcpdump_4.99.1.orig.tar.gz";
    int threadCount = 4;

    if (argc == 4 && string(argv[1]) == "-n") {
        threadCount = stoi(argv[2]);
        if (threadCount < 1) {
            cout << "Thread count must be greater than 0" << endl;
            return 1;
        }
        url = argv[3];
    } else if (argc == 2) {
        url = argv[1];
    } else if (argc == 1) {
        // cin >> url;
    } else {
        showUsage();
        return 1;
    }

    // string url = "https://mirrors.tuna.tsinghua.edu.cn/ubuntu/pool/main/t/tcpdump/tcpdump_4.99.1.orig.tar.gz";
    // string url = "http://www.baidu.com";
    // string url = "http://mirrors.tuna.tsinghua.edu.cn/index.html";
    // string url = "http://localhost/CLion-2020.3.3.tar.gz";

    // string url = "https://github.com/yuanzhouxue/auto-pppoe/archive/refs/heads/master.zip"; // 302 Found  ==> Location:
    // string url = "https://codeload.github.com/yuanzhouxue/auto-pppoe/zip/refs/heads/master";
    /*
    HTTP/1.1 200 OK
    X-GitHub-Request-Id: 2FF5:5B13:1AB45:A9564:6270992C
    Date: Tue, 03 May 2022 02:53:32 GMT
    X-XSS-Protection: 1; mode=block
    X-Frame-Options: deny
    Vary: Authorization,Accept-Encoding,Origin
    X-Content-Type-Options: nosniff
    content-disposition: attachment; filename=auto-pppoe-master.zip
    Strict-Transport-Security: max-age=31536000
    Access-Control-Allow-Origin: https://render.githubusercontent.com
    Content-Type: application/zip
    ETag: "0fe764ba8f5ccb28872402bdb31e6b08c34617fe2a0b031b96ae105b11f0a648"
    Content-Security-Policy: default-src 'none'; style-src 'unsafe-inline'; sandbox
    */

    auto start = std::chrono::system_clock::now();

    auto fileSize = multi_get::download(url, threadCount);

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

    // // auto res = conn.get(url);
    // auto res = conn.head(url);

    // res.displayHeaders();

    // // cout << res.httpVersion() << endl;
    // // cout << res.status() << endl;
    // // cout << res.statusText() << endl;
    // cout << "Body length: " << res.body().size() << endl;
    // cout << res.contentLength() << endl;

    // string filename = url.substr(url.find_last_of('/') + 1);
    // if (filename.empty()) filename = "multi-get.downloaded";

    // // cout << filename << endl;
    // ofstream out(filename);
    // out.write(res.body().data(), res.body().size());
    // out.close();

    // cout << res.contentLength() << endl;

    // string resBody{res.body().begin(), res.body().end()};
    // cout << resBody << endl;

#ifdef _WIN32
    WSACleanup();
#endif // _WIN32

    return 0;
}
