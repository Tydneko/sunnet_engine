#include "Service.h"
#include "Sunnet.h"
#include <iostream>
#include <unistd.h>
#include <cstring>

Service::Service(){
    //msg queue lock
    pthread_spin_init(&queueLock, PTHREAD_PROCESS_PRIVATE);
    //service in global queue lock
    pthread_spin_init(&inGlobalQueLock_srv, PTHREAD_PROCESS_PRIVATE);
}

Service::~Service(){
    pthread_spin_destroy(&queueLock);
    pthread_spin_destroy(&inGlobalQueLock_srv);
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
    if(msg->type == BaseMsg::TYPE::SOCKET_ACCEPT)
    {
        auto m = dynamic_pointer_cast<SocketAcceptMsg>(msg);
        cout << "new conn " << m->clientFd << endl;
    }
    if(msg->type == BaseMsg::TYPE::SOCKET_RW)
    {
        auto m = dynamic_pointer_cast<SocketRWMsg>(msg);
        if(m->isRead)
        {
            char buff_read[256];
            int len_read = read(m->fd, &buff_read, 256);
            if(len_read > 0)
            {
                char buff_write[4+len_read] = {"get:"};
                strcat(buff_write, buff_read);
                strcat(buff_write,"\n");
                write(m->fd, &buff_write, 4+len_read);
            }else{
                cout << "close " << m->fd << strerror(errno) << endl;
                Sunnet::inst->CloseConn(m->fd);
            }
        }
    }
}

void Service::OnExit(){
    cout << "[" << id_srv << "] OnExit" << endl;
}

void Service::SetSrvInGlobalQue(bool state)
{
    pthread_spin_lock(&inGlobalQueLock_srv);
    inGlobalQue_srv = state;
    pthread_spin_unlock(&inGlobalQueLock_srv);
}