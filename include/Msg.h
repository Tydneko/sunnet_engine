#pragma once
#include <stdint.h>
#include <memory>
using namespace std;

class BaseMsg{
public:
    enum TYPE{
        SERVICE = 1,
    };
    uint8_t type;
    char load_buf[999999]{};//检测内存泄露
    virtual ~BaseMsg();
};

class ServiceMsg : public BaseMsg {
public:
    uint32_t source;
    shared_ptr<char> buff;
    size_t size;
};