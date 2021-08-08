/**
 * @file server.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2021-08-08
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include <map>
#include <string>
#include <string.h>
#include <errno.h>

#include "include/ez_rpc.hpp"
#include "server.h"
#include "../dev/api_cluster.h"

#define LOG_IN_RPC(X) {#X, (X)}

class ServerAct{
protected: 
    TestStack& stack_;
public:
    ServerAct(TestStack& stack):stack_(stack) {}
    virtual ~ServerAct() {}

    virtual bool operator()(int a,long long b,int c){
        printf("ServerAct args[%d]-[%lld]-[%d]",a,b,c);
        return false;
    };
};

class ServerAct_1:public ServerAct{
public:
    ServerAct_1(TestStack& stack):ServerAct(stack) {}
    bool operator()(int a,long long b,int c) {
        printf("ServerAct_1 args[%d]-[%lld]-[%d]",a,b,c);
        return true;
    }
};

class ServerAct_2:public ServerAct{
public:
    ServerAct_2(TestStack& stack):ServerAct(stack) {}
    bool operator()(int a,long long b,int c) {
        printf("ServerAct_2 args[%d]-[%lld]-[%d]",a,b,c);
        return true;
    }
};

class ServerAct_3:public ServerAct{
public:
    ServerAct_3(TestStack& stack):ServerAct(stack) {}
    bool operator()(int a,long long b,int c) {
        printf("ServerAct_3 args[%d]-[%lld]-[%d]",a,b,c);
        return true;
    }
};

/**
 * 仿函数调用表
 */
std::map<rpc_type,ServerAct (*)(TestStack& stack)> functor_rpc_table={
    {k_func_1, [](TestStack& stack)->ServerAct{ return ServerAct_1(stack); }},
    {k_func_2, [](TestStack& stack)->ServerAct{ return ServerAct_2(stack); }},
    {k_func_3, [](TestStack& stack)->ServerAct{ return ServerAct_3(stack); }},
};


TestStack AcceptRequest(const char* host_name)
{
    static int listen_fd = -1;

    switch(listen_fd)
    {
        case -1:{
            int len = offsetof(struct sockaddr_un, sun_path) + strlen(host_name);
            struct sockaddr_un un;

            memset(&un, 0, sizeof(un));
            unlink(host_name);

            listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
            un.sun_family = AF_UNIX;
            strcpy(un.sun_path, host_name);

            if(bind(listen_fd, (struct sockaddr *)&un, len)<0
            || listen(listen_fd, 5)<0)
            {
                printf("makr %s\n",strerror(errno));
                close(listen_fd);
                listen_fd = -1;
            }
        }
        default:{
            struct sockaddr_un un;
            socklen_t len = sizeof(un);
            int accept_fd = accept(listen_fd, (struct sockaddr *)&un, &len);

            return TestStack(accept_fd);
        }
    }
}

int main(int argc,char* argv[])
{
    /**
     * 整个参数接收过程以及调用返回都封装到RpcBind中完成
     */
    while(1)
    {
        TestStack stack = AcceptRequest(SERVER_NAME);

        if(stack)
        {
            const rpc_type& symbol = ez_rpc::getArgs<0>(ez_rpc::RpcArgs<rpc_type>(stack));
    #if 0
            //从表里创建对应统一接口抽象的函数/仿函数,可在其内部对stack进行接管
            ez_rpc::bind(
                functor_rpc_table[symbol](stack)
            )(stack);
    #elif 0
            //单独使用ez_rpc::RpcArgs进行参数接收以及自由使用,注意的是列表后面的参数是先接收的
            ez_rpc::RpcArgs<int,float,bool> args(stack);
            int&& arg1 = ez_rpc::getArgs<0>(args);
            float&& arg2 = ez_rpc::getArgs<1>(args);
            bool&& arg3 = ez_rpc::getArgs<2>(args);
    #else
            printf("Start from Call[%d]\n",symbol);
            switch(symbol){
                case k_func_3:{
                    ez_rpc::bind(
                        [](int a,float b,bool c)->int {printf("Args From client=>%d-%f-%s\n",a,b,c?"true":"false");return 0;}
                    )(stack);
                    break;
                }
                case k_func_2:{
                    ez_rpc::bind(
                        [](float b,bool c)->int {printf("Args From client=>%f-%s\n",b,c?"true":"false");return 0;}
                    )(stack);
                    break;
                }
                case k_func_1:{
                    ez_rpc::bind(
                        [](bool c)->int {printf("Args From client=>%s\n",c?"true":"false");return 0;}
                    )(stack);
                    break;
                }
            }
        }
        else
        {
            printf("Server Err!\n");
            sleep(1);
        }
#endif
    }

    return 0;
}
