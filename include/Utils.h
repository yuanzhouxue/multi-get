#ifndef __UTILS_H
#define __UTILS_H

#include <string>

#include "CommonDefines.h"

std::tuple<std::string, std::string, Port, std::string> formatHost(const std::string &url);

#endif