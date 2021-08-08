# EzRpc
使用模版封装(非侵入式)来简单完成的远程函数级调用(跨主机或进程)
Simple way to do remote funciton call.

# Quick Start(快速使用)

使用模版来进行统一化的服务入口选择，传参和返回值获取步骤，减小远程调用客户端和服务端之间的通信开发成本。

## Server(服务端)

服务端需要建立一个服务入口，其不包含在本例子当中，本例假设通过`AcceptRequest`已经收到服务请求，并生成通信栈`TestStack`.  
通过`ez_rpc::bind`建立一个动作代理对象`ez_rpc::RpcServerProxyBase`  
并通过注入通信栈`TestStack`到其仿函数中来完成后续的函数参数接收调用和返回。

``` C++
using rpc_type = enum {
    k_func_1 = 0,
    k_func_2 = 1,
    k_func_3 = 2,
};

int main(int argc,char* argv[])
{
    /**
     * 整个参数接收过程以及调用返回都封装到RpcBind中完成
     */
    TestStack stack = AcceptRequest(SERVER_NAME);

    if(stack)
    {
        /**
         * 获取rpc_type,明确调用函数
         */
        const rpc_type& symbol = ez_rpc::getArgs<0>(ez_rpc::RpcArgs<rpc_type>(stack));

        /**
         * 不同的函数声明形式使用switch做匹配，如果是统一抽象的函数接口，可以使用映射表来管理。
         * 也可以把stack注入进去，后面继续使用stack进行交互。
         */
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

    return 0;
}
```

## 调用端使用的SDK API封装(服务端提供功能调用的接口封装)

对应上一节服务端存在存在以下功能接口和调用约定:

``` C++
int (*)(int a,float b,bool c);  // 调用功能约定k_func_1
int (*)(float b,bool c);        // 调用功能约定k_func_2
int (*)(bool c);                // 调用功能约定k_func_3
```

创建函数调用代理 `ez_rpc::RpcCallProxy<T>` 其中模版类型`T`为`返回值类型`,构造函数形参为注入的`通信栈`和服务端函数`调用约定标识`（本例中即为枚举量，
通过调用代理的仿函数来直接进行传参,并获取返回值就和真实函数行为一样,通过异常捕获来获取通信过程中的异常。

以上步骤即可完成和服务端通信。

``` C++

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

```

# Stack(通信栈)

通过往客户代理对象和服务代理对象中注入`ez_rpc::RpcStack`来按需完成实际的参数和返回值通信操作。  
`ez_rpc::RpcStack`已经封装了实际操作的文件描述符和判断描述符有效性的方法，其余方法允许继承动态重载，可替换以下几个函数:
1. `virtual bool getReturn(void* ret,unsigned int size);`
2. `virtual bool popReturn(const void* ret,unsigned int size);`
3. `virtual bool pushStack(void* ret,unsigned int size);`
4. `virtual bool popStack(void* ret,unsigned int size);`


# Limit(限制)

远程调用限制：
1. 被代理函数的形参必须是POD类型的数据,且为值传递.
    + 由于RPC调用限制,故参数为值传递
2. 形参不能声明为数组，如`char a[20]`
    + 因为数组在模版化后会被转化为指针,可以将数组封装进结构体以解决此缺陷
3. 参数大小需要考虑线程栈
    + 服务端接收的参数依然保存在栈中，不能大到栈溢出