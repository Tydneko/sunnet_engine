#include "Sunnet.h"
#include <unistd.h>

void test(){
    auto pingType = make_shared<string>("ping");
    uint32_t ping1 = Sunnet::inst->NewService(pingType);    
    uint32_t ping2 = Sunnet::inst->NewService(pingType);
    uint32_t pong = Sunnet::inst->NewService(pingType);

    auto msg1 = Sunnet::inst->MakeMsg(ping1, new char[3]{'h','i','\0'}, 3);
    auto msg2 = Sunnet::inst->MakeMsg(ping2, new char[6]{'h','e','l','l','o','\0'}, 6);

    Sunnet::inst->Send(pong, msg1);
    Sunnet::inst->Send(pong, msg2);
}

void testScoket()
{
    int fd = Sunnet::inst->Listen(7070, 1);
    sleep(10);
    Sunnet::inst->CloseConn(fd);
}

void testEcho()
{
    auto echoType = make_shared<string>("echo");
    uint32_t echo1 = Sunnet::inst->NewService(echoType);
    Sunnet::inst->SetServicePort(7070, echo1);
}

int main(){
    new Sunnet();
    Sunnet::inst->Start();
    //test();
    //testScoket();
    testEcho();
    Sunnet::inst->Wait();
    return 0;
}