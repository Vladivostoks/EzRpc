/**
 * @file client.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2021-07-18
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "api_cluster.h"

int main(int argc,char* argv[])
{
    int a = 0;
    char b[20] = {0};
    float c = 1.0f;
    bool d = true;
    int ret = 0;

    int rpc_rc = api_1(a,b,c,d);
}
