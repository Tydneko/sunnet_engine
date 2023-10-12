#include "Service.h"
#include "Sunnet.h"
#include <iostream>
#include <unistd.h>

Service::Service(){
    pthread_spin_init(&queueLock, PTHREAD_PROCESS_PRIVATE);
}

Service::~Service(){
    pthread_spin_destroy(&queueLock);
}

void Service::PushMsg(shared_ptr<BaseMsg> msg){
    pthread_spin_lock(&queueLock);
    msgQueue.push(msg);
    pthread_spin_unlock(&queueLock);
}

shared_ptr<BaseMsg> Service::PopMsg(){
    shared_ptr<BaseMsg> msg = nullptr;
    pthread_spin_lock(&queueLock);
    if(!msgQueue.empty()){
        msg = msgQueue.front();
        msgQueue.pop();
    }
    pthread_spin_unlock(&queueLock);
    return msg;
}

bool Service::ProcessMsg(){
    shared_ptr<BaseMsg> msg = PopMsg();
    if(msg != nullptr){
        OnMsg(msg);
        return true;
    }
    return false;//msgQueue empty
}

void Service::ProcessMsgs(int max){
    for(int i = 0; i < max; i++){
        bool succ = ProcessMsg();
        if(!succ){
            break;
        }
    }
}

//TODO

void Service::OnInit(){
    cout << "[" << id_srv << "] OnInit" << endl;
}

void Service::OnMsg(shared_ptr<BaseMsg> msg){
        cout << "[" << id_srv << "] OnMsg" << endl;
}

void Service::OnExit(){
        cout << "[" << id_srv << "] OnExit" << endl;
}