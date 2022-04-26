#include <cerrno>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <chrono>

#include "HTTPResponse.h"
#include "HTTPConnection.h"

using namespace std;


namespace multi_get {

void download(const string& url, size_t beginPos, size_t endPos) {
    multi_get::HTTPConnection conn{};
    std::stringstream ss;
    ss << "bytes=" << beginPos << '-' << endPos;
    conn.setHeader("Range", ss.str());
    auto res = conn.get(url);
    res.displayHeaders();
    std:: cout << "Downloaded " << res.contentLength() << " bytes" << std::endl;
}

void download(const string& url, size_t threadCount = 1) {
    if (threadCount < 1) threadCount = 1;
    if (threadCount > 32) threadCount = 32;

    multi_get::HTTPConnection conn{};
    auto res = conn.head(url);

    res.displayHeaders();

    if (res["Accept-Ranges"] != string("bytes")) {
        std::cout << "服务器不支持范围请求，即将使用单线程下载！" << std::endl;
        threadCount = 1;
    }

    int fileSize = std::stoi(res["Content-Length"]);
    cout << fileSize << endl;

    // 必须注意到Ranges两端都是闭区间
    int perThreadSize = fileSize / threadCount;
    int remain = fileSize % threadCount;
    int idx = -1;
    for (int i = 0; i < threadCount; ++i) {
        int range = perThreadSize;
        if (remain-- > 0) ++range;
        cout << "Thread ranges: " << idx + 1<< "-" << idx + range << endl;
        download(url, idx + 1, idx + range);
        idx += range;
    }
}

}


int main(int, char **) {
    
    string url = "http://mirrors.tuna.tsinghua.edu.cn/ubuntu/pool/main/t/tcpdump/tcpdump_4.99.1.orig.tar.gz";
    // string url = "http://mirrors.tuna.tsinghua.edu.cn/index.html";

    multi_get::download(url, 32);

    // auto start = std::chrono::system_clock::now();
    // // auto res = conn.get(url);
    // auto res = conn.head(url);
    // auto end = std::chrono::system_clock::now();
    // auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    
    
    // res.displayHeaders();

    // auto secondsUsed = double(duration.count()) * chrono::microseconds::period::num / chrono::microseconds::period::den;
    // cout << "花费了" << secondsUsed << "s" << endl;
    // cout << "平均下载速度：" << res.body().size() / secondsUsed / 1024 << " kB/s" << endl;


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
