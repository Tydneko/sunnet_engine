#include "Worker.h"
#include <iostream>
#include <unistd.h>
using namespace std;

void Worker::operator()(){
    while(true){
        cout << "working id :" << this->id_worker << endl;
        usleep(500000);//0.5s
    }
}