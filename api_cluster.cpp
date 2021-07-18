/**
 * @file api_cluster.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2021-07-18
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "include/ez_rpc.hpp"

#define CALL_RPC 1

int api_1(int a,char b[20],float c,bool d)
{
    int ret = 0;

#if CALL_RPC
    ez_rpc::RpcCall<int> rpc_func("test.socket",__func__);

    ret = rpc_func(a,b,c,d);
#else
    ret = 1;
#endif

    return ret;
}
