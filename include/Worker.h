#pragma once
#include <thread>

class Sunnet;
using namespace std;

class Worker{
public:
    int id_worker;
    int eachNume;       //每次处理多条消息
    void operator()();
};