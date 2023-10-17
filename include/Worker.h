#pragma once
#include <thread>
#include "Sunnet.h"

class Sunnet;
using namespace std;

class Worker{
public:
    int id_worker;
    int eachNume;       //每次处理多条消息
    void operator()();
    void CheckAndPutGlobal(shared_ptr<Service> srv);
};