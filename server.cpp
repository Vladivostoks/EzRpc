#include <map>
#include <string>

#include "include/ez_rpc.hpp"

#include "api_cluster.h"

#define LOG_IN_RPC(X) {#X, (X)}

#define SERVER_NAME "test.socket"

/**
 * 注册函数到远程调用表中
 */
std::map<std::string,ez_rpc::RpcServerProxyBase*> rpc_table={
    {"api_1", ez_rpc::CreateRpcProxyPoint(api_1)}
};

int main(int argc,char* argv[])
{
    /**
     * 整个参数接收过程以及调用返回都封装到RpcBind中完成
     */
    //while(1)
    {
        ez_rpc::RpcStack stack(SERVER_NAME);
        std::string symbol = "api_1";

        //略过得到handel以及symbol的步骤
        auto iter = rpc_table.find(symbol);
        if(iter != rpc_table.end()) 
        {
            ez_rpc::RpcServerProxyBase& rpc_proxy = *(iter->second);
            rpc_proxy(stack);
        }
    }

    return 0;
}
