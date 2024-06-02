
#include <iostream>

#include "Utils.h"
#include "Logger.h"


std::tuple<std::string, std::string, Port, std::string> formatHost(const std::string &url) {
    std::string protocol;
    std::string hostname;
    std::string path;
    uint16_t port = 0;

    size_t beginIdx;
    if (url.substr(0, 7) == "http://") {
        protocol = "http";
        beginIdx = 7;
    } else if (url.substr(0, 8) == "https://") {
        protocol = "https";
        beginIdx = 8;
    } else {
        LOG_ERROR("Unsupported url: %s", url.c_str());
        std::cerr << "Unsupported url: " << url << ". The url should starts with http:// or https://" << std::endl;
        exit(EXIT_FAILURE);
    }

    auto pathBeginIdx = url.find_first_of('/', beginIdx);
    if (pathBeginIdx == std::string::npos) {
        path = "/";
    } else {
        path = url.substr(pathBeginIdx);
    }
    auto hostPart = url.substr(beginIdx,  pathBeginIdx - beginIdx);
    if (auto idx = hostPart.find_first_of(':');
        idx != std::string::npos) {
        hostname = hostPart.substr(0, idx);
        port = std::stoi(hostPart.substr(idx + 1));
    } else {
        hostname = hostPart;
        port = protocol == "https" ? 443 : 80;
    }

    return {protocol, hostname, static_cast<Port>(port), path};
}