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

#include "HTTPSConnection.h"
#include "HTTPResponse.h"

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
        std::cout << "服务器不支持范围请求，即将使用单线程下载！" << std::endl;
        threadCount = 1;
    }

    int fileSize = std::stoi(res["Content-Length"]);

    // 必须注意到Ranges两端都是闭区间
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

int main(int, char **) {

    string url = "https://mirrors.tuna.tsinghua.edu.cn/ubuntu/pool/main/t/tcpdump/tcpdump_4.99.1.orig.tar.gz";
    // string url = "http://mirrors.tuna.tsinghua.edu.cn/index.html";
    // string url = "http://localhost/CLion-2020.3.3.tar.gz";

    auto start = std::chrono::system_clock::now();

    auto fileSize = multi_get::download(url, 4);

    auto end = std::chrono::system_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    auto secondsUsed = double(duration.count()) * chrono::microseconds::period::num / chrono::microseconds::period::den;
    cout << "花费了" << secondsUsed << "s" << endl;

    auto KBps = fileSize / secondsUsed / 1024.0;
    if (KBps < 1024)
        cout << "平均下载速度：" << KBps << " KB/s" << endl;
    else if (KBps < 1024 * 1024)
        cout << "平均下载速度：" << KBps / 1024.0 << " MB/s" << endl;
    else if (KBps < 1024 * 1024 * 1024)
        cout << "平均下载速度：" << KBps / 1024.0 / 1024.0 << " GB/s" << endl;

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
}
