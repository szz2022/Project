#pragma once
#include "../msgqueue/mqtest.h"
#include"../operation/operation.h"
#include "../logger/logger.h"
#include "../socket/socket.h"
#include <string>
class validity
{
private:
    /* data */
public:
    validity(/* args */);
    ~validity();
    static bool check_validity();
    static int compare_frame_num(int a,int b,int c,int d);
};
