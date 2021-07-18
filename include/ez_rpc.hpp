/**
 * @file ez_rpc.hpp
 * @author Ayden.Shu (neolandg@gmail.com)
 * @brief Easy for function rpc progress, head only
 * @version 0.1
 * @date 2021-07-18
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef __EZ_RPC_H__
#define __EZ_RPC_H__

#include <iostream>

#define RPC_DBG(X) do{\
    std::cout<<X<<std::endl;\
}while(0)

namespace ez_rpc {

class RpcStack {
public:
    RpcStack(const char* rpc_host) {
    };

    ~RpcStack() {
    };

    int funcOpen(const char* symbol) {
        RPC_DBG("symbol:"<<symbol);
        return 0;
    }

    int getReturn(void* ret,unsigned int size) {
        RPC_DBG("Get return size:"<<size);
        return 0;
    }

    int popReturn(const void* ret,unsigned int size) {
        RPC_DBG("Pop return size:"<<size);
        return 0;
    }

    int pushStack(void* ret,unsigned int size) {
        RPC_DBG("param push stack size:"<<size);
        return 0;
    }

    int popStack(void* ret,unsigned int size) {
        RPC_DBG("param pop stack size:"<<size);
        return 0;
    }
};

/**
 * @brief 压栈
 */
template<class RET,class Stack=RpcStack>
class RpcCall {
private:
    //远程栈
    Stack stack_;
    //调用符号
    const char* symbol_;

    template<class ... __ARGS>
    RET call(__ARGS... args);

    template<>
    RET call<>() {
        //参数压栈结束,等待结果返回
        RET ret;

        stack_.getReturn(&ret,sizeof(RET));
        return ret;
    }

    template<class __THIS_ARG,class ... __ARGS>
    RET call(__THIS_ARG first_arg,__ARGS... args) {
        //TODO:压栈第一个参数
        if(stack_.pushStack(&first_arg,sizeof(__THIS_ARG))<0)
        {
            throw -1;
        }

        return call(args...);
    }

public:
    RpcCall(const char* host,const char* symbol):stack_(host),
                                                 symbol_(symbol)
    {}
    ~RpcCall(){}

    template<class ... __ARGS>
    RET operator()(__ARGS... args) {
        //TODO push function symbol
        stack_.funcOpen(symbol_);

        return call(args...);
    }
};


/**
 * @brief 出栈
 */
// tuple原始版本
template <class ... __ARGS_TYPE>
class RpcArgs;

// 无参栈
template <>
class RpcArgs <> {
private:
    //通信栈
    RpcStack& stack_;

public:
    RpcArgs(RpcStack& stack):stack_(stack) {}
    virtual ~RpcArgs() {}
};

template <class __THIS_TYPE, class ... __ARGS_TYPE>
class RpcArgs <__THIS_TYPE, __ARGS_TYPE ...> : public RpcArgs<__ARGS_TYPE ...> {
private:
    //每一层继承递归构成参数栈
    __THIS_TYPE value_;

    //通信栈
    RpcStack& stack_;

public:
    RpcArgs(RpcStack& stack) : RpcArgs<__ARGS_TYPE ...>(stack),
                               stack_(stack)
    {
        stack_.popStack(&value_,sizeof(__THIS_TYPE));
    }
    virtual ~RpcArgs() {}
 
    __THIS_TYPE getValue()
    {
        return value_;
    }
};

/**
 * @brief 实现从参数栈中取出第N个参数
 * 
 * @tparam N 
 * @tparam __ARGS_TYPE 
 */
template <int N, class ... __ARGS_TYPE>
struct element;

// N > size时get出错
template <int N>
struct element<N, RpcArgs<>> {
    static_assert(0 > N, "Index outside of argument stack!");
};

// 展开到对应节点
template <class __THIS_TYPE, class ... __ARGS_TYPE>
struct element<0, RpcArgs<__THIS_TYPE, __ARGS_TYPE ...>>
{
    using value_t = __THIS_TYPE;
    using class_t = RpcArgs<__THIS_TYPE, __ARGS_TYPE ...>;
};

// 展开模板，通过对自身偏特化版本“继承”，从而获得对应的节点类型
template <int N, class __THIS_TYPE, class ... __ARGS_TYPE>
struct element<N, RpcArgs<__THIS_TYPE, __ARGS_TYPE ...>> 
: public element<N - 1, RpcArgs<__ARGS_TYPE ...>> {
};

template <int N, class ... __ARGS_TYPE>
auto getArgs(RpcArgs<__ARGS_TYPE ...>& args) {
    using __class_t = typename element<N, RpcArgs<__ARGS_TYPE ...>>::class_t;
    return ((__class_t &)args).getValue();
}

template<class T>
class args_size;

template<class ... __ARGS>
class args_size<RpcArgs<__ARGS...>>{
public:
    static constexpr const unsigned int value = sizeof...(__ARGS);
};

class RpcBindBase{
public:
    virtual int operator()(RpcStack& stack)=0;
};

template<class __RET_TYPE,class ... __ARGS_TYPE>
class RpcBind:public RpcBindBase{
private: 
    // 通过绑定函数的模版类型进行萃取
    using func_t =  __RET_TYPE (*)(__ARGS_TYPE...);
    
    // 被绑定函数
    func_t bind_func_;

    //参数展开
    template<class __ARGS, std::size_t... I>
    __RET_TYPE process(__ARGS& args,std::index_sequence<I...>){
        return bind_func_(getArgs<I>(args)...);
    }

    __RET_TYPE process(RpcArgs<__ARGS_TYPE...>& args){
        return process(args,
                       std::make_index_sequence<args_size<RpcArgs<__ARGS_TYPE...>>::value>());
    }
public:
    RpcBind(func_t bind_func):bind_func_(bind_func)
    {
    };

    virtual ~RpcBind(){
    };

    int operator()(RpcStack& stack)
    {
        try{
            //step1: 等待远程参数接收
            RpcArgs<__ARGS_TYPE...> Args(stack);

            //step2: 参数展开,并执行
            const __RET_TYPE& ret = process(Args);

            //step3: 返回调用者
            stack.popReturn(&ret,sizeof(__RET_TYPE));
        }catch(...){
            return -1;
        }


        return 0;
    }
};


} // namespace name

#endif
