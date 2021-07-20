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
#include <tuple>

#define RPC_DBG(X) do{\
    std::cout<<X<<std::endl;\
}while(0)

namespace ez_rpc {

/**
 * @brief Rpc Err code
 */
using rpc_err_t = enum RPC_ERR {
    k_rpc_ok = 0,
    k_rpc_func_not_found = -1,
    k_rpc_param_push_failed = -2,
    k_rpc_ret_get_failed = -3
};

/**
 * @brief Communicate Stack from rpc caller to server
 */
class RpcStack {
public:
    RpcStack(const char* rpc_host) {
    };

    virtual ~RpcStack() {
    };

    virtual bool funcOpen(const char* symbol) {
        RPC_DBG("Push symbol:"<<symbol);
        return true;
    }

    virtual bool getReturn(void* ret,unsigned int size) {
        RPC_DBG("Get return size:"<<size);
        return true;
    }

    virtual bool popReturn(const void* ret,unsigned int size) {
        RPC_DBG("Pop return size:"<<size);
        return true;
    }

    virtual bool pushStack(void* ret,unsigned int size) {
        RPC_DBG("param push stack size:"<<size);
        return true;
    }

    virtual bool popStack(void* ret,unsigned int size) {
        RPC_DBG("param pop stack size:"<<size);
        return true;
    }
};

/**
 * @brief Caller Decorator
 * 
 * @tparam RET Remote function ret type
 */

template<class RET>
class RpcCallProxy {
private:
    RpcStack& stack_;

    const char* symbol_;

    template<class ... __ARGS>
    bool pushStack(__ARGS... args) {
        return true;
    }

    template<class __THIS_ARG,class ... __ARGS>
    bool pushStack(__THIS_ARG first_arg,__ARGS... args) {
        bool ret = pushStack(args...);
        
        //Push Current Argument
        ret &= stack_.pushStack(&first_arg,sizeof(__THIS_ARG));

        return ret;
    }

public:
    RpcCallProxy(RpcStack& stack,const char* symbol):stack_(stack),
                                                     symbol_(symbol)
    {}
    ~RpcCallProxy(){}

    template<class ... __ARGS>
    RET operator()(__ARGS... args) {
        RET ret;
        
        /**
         * Open function ,push arguments and get ret value like a local function
         */
        if(!stack_.funcOpen(symbol_))
        {
            throw k_rpc_func_not_found;
        }

        if(!pushStack(args...))
        {
            throw k_rpc_param_push_failed;
        }

        if(!stack_.getReturn(&ret,sizeof(RET)))
        {
            throw k_rpc_ret_get_failed;
        }

        return ret;
    }
};


/**
 * @brief Args Stack for Rpc Server
 */
template <class ... __ARGS_TYPE>
class RpcArgs;

template <>
class RpcArgs <> {
private:
    RpcStack& stack_;

public:
    RpcArgs(RpcStack& stack):stack_(stack) {}
    virtual ~RpcArgs() {}
};

template <class __THIS_TYPE, class ... __ARGS_TYPE>
class RpcArgs <__THIS_TYPE, __ARGS_TYPE ...> : public RpcArgs<__ARGS_TYPE ...> {
private:
    RpcStack& stack_;

    __THIS_TYPE value_; //Server args stack consists of per inherit RpcArgs class
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
 * @brief Get Nth value in arguments stack
 * 
 * @tparam N index of arguments
 * @tparam __ARGS_TYPE arguments types
 */
template <int N, class ... __ARGS_TYPE>
struct element;

//if N > size assert error
template <int N>
struct element<N, RpcArgs<>> {
    static_assert(0 > N, "Index outside of argument stack!");
};

// find finnal 
template <class __THIS_TYPE, class ... __ARGS_TYPE>
struct element<0, RpcArgs<__THIS_TYPE, __ARGS_TYPE ...>>
{
    using value_t = __THIS_TYPE;
    using class_t = RpcArgs<__THIS_TYPE, __ARGS_TYPE ...>;
};

// Expand arguments
template <int N, class __THIS_TYPE, class ... __ARGS_TYPE>
struct element<N, RpcArgs<__THIS_TYPE, __ARGS_TYPE ...>> 
: public element<N - 1, RpcArgs<__ARGS_TYPE ...>> {
};

// Get argument with index of N 
template <int N, class ... __ARGS_TYPE>
auto getArgs(RpcArgs<__ARGS_TYPE ...>& args) {
    using __class_t = typename element<N, RpcArgs<__ARGS_TYPE ...>>::class_t;
    return ((__class_t &)args).getValue();
}

/**
 * @brief Get Arguments nums
 * 
 * @tparam __ARGS Arguments list 
 */
template<class T>
class args_size;

template<class ... __ARGS>
class args_size<RpcArgs<__ARGS...>>{
public:
    static constexpr const unsigned int value = sizeof...(__ARGS);
};

/**
 * @brief Base Rpc Bind Class for Polumorphism in run time
 * 
 */
class RpcServerProxyBase{
public:
    RpcServerProxyBase(){}
    virtual ~RpcServerProxyBase(){}
    virtual rpc_err_t operator()(RpcStack& stack) const=0;
};

/**
 * @brief Server Bind function
 * 
 * @tparam __RET_TYPE function ret type
 * @tparam __ARGS_TYPE function arguments type list
 */
template<class __RET_TYPE,class ... __ARGS_TYPE>
class RpcServerProxy:public RpcServerProxyBase{
private: 
    using func_t =  __RET_TYPE (*)(__ARGS_TYPE...);
    
    func_t proxy_func_;

    template<class __ARGS, std::size_t... I>
    __RET_TYPE process(__ARGS& args,std::index_sequence<I...>) const {
        return proxy_func_(getArgs<I>(args)...);
    }

    __RET_TYPE process(RpcArgs<__ARGS_TYPE...>& args) const {
        return process(args,
                       std::make_index_sequence<args_size<RpcArgs<__ARGS_TYPE...>>::value>());
    }
public:
    RpcServerProxy(func_t bind_func):proxy_func_(bind_func)
    {
    };

    virtual ~RpcServerProxy(){
    };

    rpc_err_t operator()(RpcStack& stack) const
    {
        try{
            /**
             * Receive Args in RpcArgs 
             * and input to process for bind funciton
             * return value
             */
            RpcArgs<__ARGS_TYPE...> Args(stack);

            const __RET_TYPE& ret = process(Args);

            stack.popReturn(&ret,sizeof(__RET_TYPE));
        }catch(rpc_err_t err){
            return err;
        }

        return k_rpc_ok;
    }
};

/**
 * @brief Create a Rpc Bind Point object
 * 
 * @tparam __RET_TYPE proxy function ret type
 * @tparam __ARGS_TYPE proxy function args types
 * @param func proxy function
 * @return RpcServerProxy<__RET_TYPE,__ARGS_TYPE...>* 
 */
template<class __RET_TYPE,class ... __ARGS_TYPE>
RpcServerProxy<__RET_TYPE,__ARGS_TYPE...>* CreateRpcProxyPoint(__RET_TYPE (*func)(__ARGS_TYPE...))
{
    return new RpcServerProxy<__RET_TYPE,__ARGS_TYPE...>(func);
}

/**
 * @brief Create a Rpc Bind object
 * 
 * @tparam __RET_TYPE proxy function ret type
 * @tparam __ARGS_TYPE proxy function args types
 * @param func proxy function
 * @return RpcServerProxy<__RET_TYPE,__ARGS_TYPE...> 
 */
template<class __RET_TYPE,class ... __ARGS_TYPE>
RpcServerProxy<__RET_TYPE,__ARGS_TYPE...> CreateRpcProxy(__RET_TYPE (*func)(__ARGS_TYPE...))
{
    return RpcServerProxy<__RET_TYPE,__ARGS_TYPE...>(func);
}

} // namespace name

#endif
