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
    ez_rpc::RpcStack stack = ez_rpc::RpcStack("test.socket");
    ez_rpc::RpcCallProxy<int> proxy_func(stack,__func__);

    try{
        ret = proxy_func(a,b,c,d);
    }catch(ez_rpc::rpc_err_t err) {
        RPC_DBG("proxy rpc fun failed,err"<<err);
    }

#else
    /**
     * Real api_i funciton content
     */
    ret = 1;
#endif

    return ret;
}
