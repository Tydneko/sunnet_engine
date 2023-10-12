#include "Sunnet.h"
#include <iostream>
using namespace std;

Sunnet* Sunnet::inst;
Sunnet::Sunnet(){
    inst = this;
}

void Sunnet::Start(){
    cout << "Hello Sunnet Engine!" << endl;
    //rwlock
    pthread_rwlock_init(&servicesLock, NULL);
    //worker
    StartWorkers();
}

void Sunnet::StartWorkers(){
    for(int i = 0; i < WORKER_NUM; i++){
        cout << "start worker thread : " << i << endl;

        Worker* worker = new Worker();
        worker->id_worker = i;
        worker->eachNume = 2 << i;//2^i 每次处理多少条消息
        thread* wt = new thread(*worker);

        worker_vec.push_back(worker);
        workerThread_vec.push_back(wt);
    }
}

void Sunnet::Wait(){
    if(workerThread_vec[0]){
        workerThread_vec[0]->join();
    }
}

uint32_t Sunnet::NewService(shared_ptr<string> type){
    auto srv = make_shared<Service>();
    srv->type_srv = type;

    pthread_rwlock_wrlock(&servicesLock);
    srv->id_srv = maxId_srv;
    maxId_srv++;
    services_map.emplace(srv->id_srv, srv);
    pthread_rwlock_unlock(&servicesLock);

    srv->OnInit();
    //TODO init失败
    return srv->id_srv;
}

void Sunnet::QuitService(uint32_t id){
    shared_ptr<Service> srv = GetService(id);
    if(!srv){
        return ;//不存在
    }

    srv->OnExit();
    srv->isExiting = true;
    //TODO exit失败
    
    pthread_rwlock_wrlock(&servicesLock);
    services_map.erase(id);
    pthread_rwlock_unlock(&servicesLock);
}

shared_ptr<Service> Sunnet::GetService(uint32_t id){
    shared_ptr<Service> srv = nullptr;

    pthread_rwlock_rdlock(&servicesLock);
    auto iter = services_map.find(id);
    if(iter != services_map.end()){
        srv = iter->second;
    }
    pthread_rwlock_unlock(&servicesLock);

    return srv;
}

