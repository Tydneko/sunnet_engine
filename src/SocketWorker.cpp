#include "SocketWorker.h"
#include <iostream>
#include <unistd.h>
#include <assert.h>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>

#define EPOLL_CREATE_NUM 1024 // useless

void SocketWorker::Init()
{
    cout << "SocketWorker Init " << endl;
    epollFd = epoll_create(EPOLL_CREATE_NUM);
    assert(epollFd > 0);
}

void SocketWorker::operator()()
{
    while (true)
    {
        cout << "SocketWork "<< endl;
        const int EVENT_SIZE = 64;
        struct epoll_event events[EVENT_SIZE];
        int eventCount = epoll_wait(epollFd, events, EVENT_SIZE, -1);// -1超时时间 ，-1陷入等待

        for(int i = 0; i < eventCount; i++)
        {
            epoll_event ev = events[i];
            OnEvent(ev);
        }

        usleep(100000);
    }
    
}

void SocketWorker::AddEvent(int fd)
{
    cout << "AddEvent fd : " << fd << endl;

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = fd;
    /*
        sunnet_engine 默认采用边缘触发(EPOLLET)
        水平触发：如果没有一次性完成读写操作，下次调用epoll_wait时，操作系统还会发出通知
        边缘触发：同样的情况，操作系统只会通知一次
    */
    if(epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        cerr << "AddEvent epoll_ctl Fail : " << strerror(errno) << endl;
    }
}

void SocketWorker::RemoveEvent(int fd)
{
    cout << "RemoveEvent fd : " << fd << endl;
    epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL);
}

void SocketWorker::ModifyEvent(int fd, bool epollOut)
{
    cout << "ModifyEvent fd : " << fd << " " << epollOut << endl;

    struct epoll_event ev;
    ev.data.fd = fd;
    if(epollOut)
    {
        ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    }else{
        ev.events = EPOLLIN | EPOLLET;
    }
    epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &ev);
}

void SocketWorker::OnEvent(epoll_event ev)
{
    cout << "OnEvent " << endl;
    int fd = ev.data.fd;
    auto conn = Sunnet::inst->GetConn(fd);
    if(conn == nullptr)
    {
        cout << "OnEvent error, get conn error." << endl;
        return;
    }

    bool isRead = ev.events & EPOLLIN;
    bool isWrite = ev.events & EPOLLOUT;
    bool isError = ev.events & EPOLLERR;

    if(conn->type == Conn::TYPE::LISTEN)
    {
        if(isRead)
            OnAccept(conn);
    }else if(conn->type == Conn::TYPE::CLIENT){
        if(isRead || isWrite)
            OnReadWrite(conn, isRead, isWrite);
        if(isError)
            cout << "OnError fd : " << conn->fd << endl;
    }
}

void SocketWorker::OnAccept(shared_ptr<Conn> conn)
{
    cout << "OnAccept fd : " << conn->fd << endl;

    int clientFd = accept(conn->fd, nullptr, nullptr);
    if(clientFd < 0)
    {
        cout << "OnAccept error, accept client error." << endl;
    }

    //non-blocking
    fcntl(clientFd, F_SETFL, O_NONBLOCK);
    Sunnet::inst->NewConn(clientFd, conn->serviceId, Conn::TYPE::CLIENT);

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = clientFd;
    if(-1 == epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &ev))
    {
        cout << "OnAccept error, epoll_ctl fail : " << strerror(errno) << endl;
    }

    auto msg = make_shared<SocketAcceptMsg>();
    msg->type - BaseMsg::TYPE::SOCKET_ACCEPT;
    msg->listenFd = conn->fd;
    msg->clientFd = clientFd;
    Sunnet::inst->Send(conn->serviceId, msg);
}

void SocketWorker::OnReadWrite(shared_ptr<Conn> conn, bool isRead, bool isWrite)
{
    cout << "OnReadWrite fd : " << conn->fd << endl;
    auto msg = make_shared<SocketRWMsg>();
    msg->type = BaseMsg::TYPE::SOCKET_RW;
    msg->fd = conn->fd;
    msg->isRead = isRead;
    msg->isWrite = isWrite;
    Sunnet::inst->Send(conn->serviceId, msg);
}