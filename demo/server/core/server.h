/**
 * @file server.h
 * @author Ayden.Shu (neolandg@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2021-08-08
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#ifndef __SERVER_INSIDE_H__
#define __SERVER_INSIDE_H__

#include <string.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>

#include "include/ez_rpc.hpp"

#define SERVER_NAME "test.socket"

class TestStack:public ez_rpc::RpcStack
{
private:
public:
    //server function
    inline TestStack(int fd):ez_rpc::RpcStack(fd) {}
    inline TestStack(TestStack&& test_stack):ez_rpc::RpcStack(test_stack.socket_fd_) {
        test_stack.socket_fd_ = -1;
    }
    //clinet function
    inline TestStack(const char* host_name):ez_rpc::RpcStack(-1) {
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        int len = offsetof(struct sockaddr_un, sun_path) + strlen(host_name);
        struct sockaddr_un un;

        memset(&un, 0, sizeof(un));
        un.sun_family = AF_UNIX;
        sprintf(un.sun_path, "%s", host_name);

        if(connect(fd, (struct sockaddr *)&un, len) < 0)
        {
            printf("errno:%s\n",strerror(errno));
        }
        else
        {
            socket_fd_ = fd;
            fd = -1;
        }
        
        if(fd>0)
        {
            close(fd);
            fd = -1;
        }
    }
};

using rpc_type = enum {
    k_func_1 = 0,
    k_func_2 = 1,
    k_func_3 = 2,
};


#endif
