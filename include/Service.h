#pragma once
#include <queue>
#include <thread>
#include <string>
#include <Msg.h>

using namespace std;

class Service{
public:
    uint32_t id_srv;
    shared_ptr<string> type_srv;
    bool isExiting = false;
    
    queue<shared_ptr<BaseMsg>> msgQueue;
    pthread_spinlock_t queueLock;
public:
    Service();
    ~Service();

    void OnInit();
    void OnMsg(shared_ptr<BaseMsg> msg);
    void OnExit();

    void PushMsg(shared_ptr<BaseMsg> msg);
    bool ProcessMsg();
    void ProcessMsgs(int max);
private:
    shared_ptr<BaseMsg> PopMsg();

public:
    //exist in global queue
    bool inGlobalQue_srv = false;
    pthread_spinlock_t inGlobalQueLock_srv;
    void SetSrvInGlobalQue(bool state);
};