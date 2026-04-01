#pragma once

#include "../Common.h"
#include <string>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <winsock2.h>

class IParser{
  public:
    virtual ~IParser() = default;
    virtual void processBBF(const char* buffer, int32_t size) = 0;
    virtual void printStatistics()  = 0;
};