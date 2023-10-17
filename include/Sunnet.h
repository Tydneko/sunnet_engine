#pragma once 
#include <vector>
#include "Service.h"
#include "Worker.h"
#include <unordered_map>
#define WORKER_NUM 3

class Worker;

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

private:
    //全局队列
    queue<shared_ptr<Service>> globalQueue;
    int globalQueueLen = 0;
    pthread_spinlock_t gloableLock;
public:
    void Send(uint32_t toId, shared_ptr<BaseMsg> msg);
    shared_ptr<Service> PopGlobalQueue();
    void PushGlobalQueue(shared_ptr<Service> srv);
    //test
    shared_ptr<BaseMsg> MakeMsg(uint32_t source, char* buff, int len);
};