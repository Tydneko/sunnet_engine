#include "Sunnet.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

Sunnet* Sunnet::inst;
Sunnet::Sunnet(){
    inst = this;
}

void Sunnet::Start(){
    cout << "Hello Sunnet Engine!" << endl;
    //rwlock
    pthread_rwlock_init(&servicesLock, NULL);
    pthread_rwlock_init(&connsLock, NULL);

    //global queue
    pthread_spin_init(&gloableLock, PTHREAD_PROCESS_PRIVATE);

    //worker threads
    pthread_mutex_init(&sleepMtx, NULL);
    pthread_cond_init(&sleepCond, NULL);

    StartWorkers();

    //socket worker 
    StartSocketWorker();
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

void Sunnet::StartSocketWorker()
{
    socketWorker = new SocketWorker();
    socketWorker->Init();
    socketThread = new thread(*socketWorker);
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

void Sunnet::SetServicePort(uint32_t port, uint32_t serviceId)
{
    Sunnet::inst->Listen(port, serviceId);
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

int Sunnet::NewConn(int fd, uint32_t id, Conn::TYPE type)
{
    auto conn = make_shared<Conn>();
    conn->fd = fd;
    conn->serviceId = id;
    conn->type = type;

    pthread_rwlock_wrlock(&connsLock);
    conns_map.emplace(fd, conn);
    pthread_rwlock_unlock(&connsLock);
    return fd;
}

shared_ptr<Conn> Sunnet::GetConn(int fd)
{
    shared_ptr<Conn> conn = nullptr;
    pthread_rwlock_rdlock(&connsLock);
    auto iter = conns_map.find(fd);
    if(iter != conns_map.end()){
        conn = iter->second;
    }
    pthread_rwlock_unlock(&connsLock);
    return conn;
}

bool Sunnet::QuitConn(int fd)
{
    int ret;
    pthread_rwlock_rdlock(&connsLock);
    ret = conns_map.erase(fd);
    pthread_rwlock_unlock(&connsLock);
    return ret == 1;
}

//global message queue
shared_ptr<Service> Sunnet::PopGlobalQueue()
{
    shared_ptr<Service> srv = nullptr;

    pthread_spin_lock(&gloableLock);
    if(!globalQueue.empty())
    {
        srv = globalQueue.front();
        globalQueue.pop();
        globalQueueLen--;
    }
    pthread_spin_unlock(&gloableLock);

    return srv;
}

void Sunnet::PushGlobalQueue(shared_ptr<Service> srv)
{
    pthread_spin_lock(&gloableLock);
    globalQueue.push(srv);
    globalQueueLen++;
    pthread_spin_unlock(&gloableLock);
}

void Sunnet::Send(uint32_t toId, shared_ptr<BaseMsg> msg)
{
    shared_ptr<Service> toSrv = GetService(toId);
    if(!toSrv){
        cout << "Send fail, toSrv not exist. toId : " << toId << endl;
    }
    toSrv->PushMsg(msg);

    bool inGlobalQue = false;
    pthread_spin_lock(&toSrv->inGlobalQueLock_srv);
    if(!toSrv->inGlobalQue_srv)
    {
        PushGlobalQueue(toSrv);
        toSrv->inGlobalQue_srv = true;
        inGlobalQue = true;
    }
    pthread_spin_unlock(&toSrv->inGlobalQueLock_srv);
    
    //TODO 唤醒线程
    if(inGlobalQue)
    {
        CheckAndWeakUp();
    }
}

shared_ptr<BaseMsg> Sunnet::MakeMsg(uint32_t source, char* buff, int len)
{
    auto msg = make_shared<ServiceMsg>();
    msg->type = BaseMsg::TYPE::SERVICE;
    msg->source = source;

    msg->buff = shared_ptr<char>(buff);
    msg->size = len;
    return msg;
}

void Sunnet::CheckAndWeakUp()
{
    //unsafe
    if(sleepCount == 0)
    {
        return;
    }

    if(WORKER_NUM - sleepCount <= globalQueueLen)
    {
        cout << "Wakeup !" << endl;
        pthread_cond_signal(&sleepCond);
    }
}

void Sunnet::WorkerWait()
{
    pthread_mutex_lock(&sleepMtx);
    sleepCount++;
    pthread_cond_wait(&sleepCond, &sleepMtx);
    sleepCount--;
    pthread_mutex_unlock(&sleepMtx);
}

int Sunnet::Listen(uint32_t port, uint32_t serviceId)
{
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd <= 0)
    {
        cout << "Listen error, create listenfd error." << endl;
        // TODO ERR_TYPE.LISTEN.
        return -1;
    }

    //non-blocking
    fcntl(listenFd, F_SETFL, O_NONBLOCK);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int ret_bind = bind(listenFd, (struct sockaddr*)&addr, sizeof(addr));
    if(-1 == ret_bind)
    {
        cout << "Listen error, bind error." << endl;
        // TODO ERR_TYPE.LISTEN.
        return -1;
    }

    int ret_listen = listen(listenFd, 64);
    if(ret_listen < 0)
    {
        // TODO ERR_TYPE.LISTEN.
        return -1;
    }

    NewConn(listenFd, serviceId, Conn::TYPE::LISTEN);
    socketWorker->AddEvent(listenFd);
    return listenFd;
}

void Sunnet::CloseConn(uint32_t fd)
{
    bool ret_QConn = QuitConn(fd);
    close(fd);
    if(ret_QConn)
    {
        socketWorker->RemoveEvent(fd);
    }
}