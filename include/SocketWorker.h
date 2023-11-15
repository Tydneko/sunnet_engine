#pragma once
#include <sys/epoll.h>
#include <memory>
#include "Conn.h"
#include "Sunnet.h"

using namespace std;

class SocketWorker {
public:
    void Init();
    void operator()();

private:
    int epollFd;
public:
    void AddEvent(int fd);
    void RemoveEvent(int fd);
    void ModifyEvent(int fd, bool epollout);
private:
    void OnEvent(epoll_event ev);
    void OnAccept(shared_ptr<Conn> conn);
    void OnReadWrite(shared_ptr<Conn> conn, bool isRead, bool isWrite);
};