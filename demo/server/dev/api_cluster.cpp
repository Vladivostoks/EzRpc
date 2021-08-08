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

#include "../core/server.h"

#define CALL_RPC 1

int api_3(int a,float b,bool c)
{
    int ret = 0;

    try{
        TestStack stack = TestStack(SERVER_NAME);
        ez_rpc::RpcCallProxy<int> proxy_func(stack,k_func_3);
        ret = proxy_func(a,b,c);
    }catch(ez_rpc::rpc_err_t err) {
        RPC_DBG("proxy rpc fun failed,err"<<err);
    }

    return ret;
}

int api_2(float b,bool c)
{
    int ret = 0;

    try{
        TestStack stack = TestStack(SERVER_NAME);
        ez_rpc::RpcCallProxy<int> proxy_func(stack,k_func_2);
        ret = proxy_func(b,c);
    }catch(ez_rpc::rpc_err_t err) {
        RPC_DBG("proxy rpc fun failed,err"<<err);
    }

    return ret;
}

int api_1(bool c)
{
    int ret = 0;

    try{
        TestStack stack = TestStack(SERVER_NAME);
        ez_rpc::RpcCallProxy<int> proxy_func(stack,k_func_1);
        ret = proxy_func(c);
    }catch(ez_rpc::rpc_err_t err) {
        RPC_DBG("proxy rpc fun failed,err"<<err);
    }

    return ret;
}