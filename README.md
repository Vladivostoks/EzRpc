# EzRpc
使用模版封装(非侵入式)来简单完成的远程函数级调用(跨主机或进程)
Simple way to do remote funciton call.

# Quick Start(快速使用)

使用模版来进行统一化的函数选择，传参和返回值获取，减小远程调用客户端和服务端之间的通信开发成本。

假设存在以下函数

``` C++
int api_1(int a,longlong b,float c,bool d);
```

## Client(客户端)

创建函数调用代理 `ez_rpc::RpcCallProxy<T>` 其中模版值为返回值类型,形参为注入的通信栈和服务端函数调用约定标识（本例中即为函数名。 
通过调用代理的仿函数来直接进行传参,并获取返回值就和真实函数一样。

``` C++

int api_1_proxy(int a,longlong b,float c,bool d)
{
    ez_rpc::RpcStack stack = ez_rpc::RpcStack("test.socket");
    ez_rpc::RpcCallProxy<int> proxy_func(stack,"api_1");

    try{
        ret = proxy_func(a,b,c,d);
    }catch(ez_rpc::rpc_err_t err) {
        RPC_DBG("proxy rpc fun failed,err"<<err);
    }
}

```

## Server(服务端)

服务端需要建立一个服务入口，其不包含在本例子当中，本例假设已经收到服务请求.  
通过`ez_rpc::CreateRpcProxyPoint`建立一个动作代理对象`ez_rpc::RpcServerProxyBase`  
并通过注入通信栈到其仿函数中来完成后续的函数参数接收调用和返回。

``` C++
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
    while(1)
    {
        /*
         * ...
         * 假设前置步骤已经获取符号 
         *      std::string symbol = "api_1";
         * 以及建立通信栈stack
         *      ez_rpc::RpcStack stack;
         */

        auto iter = rpc_table.find(symbol);
        if(iter != rpc_table.end()) 
        {
            ez_rpc::RpcServerProxyBase& rpc_proxy = *(iter->second);
            /**
             * 注入参数栈开始接收参数和执行并返回给客户端
             */
            rpc_proxy(stack);
        }
    }

    return 0;
}
```


# Stack(通信栈)

通过往客户代理对象和服务代理对象中注入`ez_rpc::RpcStack`来按需完成实际的参数和返回值通信操作。  
`ez_rpc::RpcStack`允许继承动态重载，其必须实现以下几个函数:
1. `virtual bool funcOpen(const char* symbol);`
2. `virtual bool getReturn(void* ret,unsigned int size);`
3. `virtual bool popReturn(const void* ret,unsigned int size);`
4. `virtual bool pushStack(void* ret,unsigned int size);`
5. `virtual bool popStack(void* ret,unsigned int size);`


# Limit(限制)

远程调用限制：
1. 被代理函数的形参必须是POD类型的数据,且为值传递.
    + 由于RPC调用限制,故参数为值传递
2. 形参不能声明为数组，如`char a[20]`
    + 因为数组在模版化后会被转化为指针,可以将数组封装进结构体以解决此缺陷