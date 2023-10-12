#pragma once 
#include <vector>
#include "Worker.h"
#include "Service.h"
#include <unordered_map>
#define WORKER_NUM 3

class Sunnet{
public:
    static Sunnet *inst;
public:
    Sunnet();
    void Start();
    void Wait();
private:
    int workerNum = WORKER_NUM;
    vector<Worker*> worker_vec;
    vector<thread*> workerThread_vec;
private:
    void StartWorkers();

public:
    unordered_map<uint32_t, shared_ptr<Service>> services_map;
    uint32_t maxId_srv = 0;
    pthread_rwlock_t servicesLock;
public:
    uint32_t NewService(shared_ptr<string> type);
    void QuitService(uint32_t id);
private:
    shared_ptr<Service> GetService(uint32_t id);
};