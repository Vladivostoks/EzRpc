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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <iostream>
#include <tuple>
#include <type_traits>

#if 0
#define RPC_DBG(X)                                                                 \
    do {                                                                           \
        std::cout << "[" << __FILE__ << "][" << __LINE__ << "]" << X << std::endl; \
    } while (0)
#else
#define RPC_DBG(X)
#endif

namespace ez_rpc {

/**
 * @brief Rpc Err code
 */
using rpc_err_t = enum RPC_ERR {
    k_rpc_ok = 0,
    k_rpc_func_call_failed = -1,
    k_rpc_param_push_failed = -2,
    k_rpc_ret_get_failed = -3,
    k_rpc_callfun_interrupt = -4,
};

/**
 * @brief Communicate Stack from rpc caller to server
 */
class RpcStack {
protected:
    // communicate socket fo
    int socket_fd_;

public:
    RpcStack(int socket_fd):socket_fd_(socket_fd) { };
    RpcStack(RpcStack&& stack):socket_fd_(stack.socket_fd_){
        stack.socket_fd_ = -1;
    }
    RpcStack(const char* host_name, bool anonymity = true):socket_fd_(-1){
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        int len = offsetof(struct sockaddr_un, sun_path) + strlen(host_name);
        struct sockaddr_un un;

        memset(&un, 0, sizeof(un));

        un.sun_family = AF_UNIX;
        if (anonymity) {
            strncpy(un.sun_path + 1, host_name, sizeof(un.sun_path) - 1);
            len += 1;
        } else {
            strncpy(un.sun_path, host_name, sizeof(un.sun_path) - 1);
        }

        if (fd >= 0) {
            if (connect(fd, (struct sockaddr*)&un, len) == 0) {
                socket_fd_ = fd;
                fd = -1;
            } else {
                RPC_DBG("errno:" << errno << ":" << strerror(errno));
            }
        }

        /*f fd is not picked up, close it */
        if (fd >= 0) {
            close(fd);
            fd = -1;
        }
    }

    virtual ~RpcStack()
    {
        if (socket_fd_ >= 0) {
            close(socket_fd_);
            socket_fd_ = -1;
        }
    };

    /**
     * check socket is invalid or not
     */
    bool operator!() { return socket_fd_ < 0; }

    operator bool() { return socket_fd_ >= 0; }

    /**
     * @brief detach socket handle from object
     */
    int detach_native_handle() && {
        int ret = socket_fd_;
        socket_fd_ = -1;
        return ret;
    }

    virtual bool popReturn(void* ret, unsigned int size) {
        RPC_DBG("Pop return size:" << size);
        unsigned int offset = 0;
        int rc = 0;

        do {
            rc = read(socket_fd_, (char*)ret + offset, size - offset);
            offset += rc > 0 ? rc : 0;
        } while ((rc < 0 && errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) || (rc > 0 && (size - offset) > 0));

        return size == offset ? true : false;
    }

    virtual bool pushReturn(const void* ret, unsigned int size) {
        RPC_DBG("Push return size:" << size);
        unsigned int offset = 0;
        int rc = 0;

        do {
            rc = write(socket_fd_, (char*)ret + offset, size - offset);
            offset += rc > 0 ? rc : 0;
        } while ((rc < 0 && errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) || (rc > 0 && (size - offset) > 0));

        return rc < 0 ? false : true;
    }

    virtual bool pushStack(const void* ret, unsigned int size) {
        RPC_DBG("param push stack size:" << size);
        unsigned int offset = 0;
        int rc = 0;

        do {
            rc = write(socket_fd_, (char*)ret + offset, size - offset);
            offset += rc > 0 ? rc : 0;
        } while ((rc < 0 && errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) || (rc > 0 && (size - offset) > 0));

        return rc < 0 ? false : true;
    }

    virtual bool popStack(void* ret, unsigned int size) {
        RPC_DBG("param pop stack size:" << size);
        unsigned int offset = 0;
        int rc = 0;

        do {
            rc = read(socket_fd_, (char*)ret + offset, size - offset);
            offset += rc > 0 ? rc : 0;
        } while ((rc < 0 && errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) || (rc > 0 && (size - offset) > 0));

        return size == offset ? true : false;
    }
};

/**
 * @brief auto listen&accept on an unix address
 *
 * @param listen_fd listen_fd, stored outside
 * @param ms_timeout host addr
 * @param host_name  accept ms timeout, forever is -1
 * @param anonymity is anonymity unix socket
 * @return RpcStack
 */
inline RpcStack AcceptRequest(int& listen_fd, 
                              int ms_timeout = -1,
                              const char* host_name = nullptr,
                              bool anonymity = true) {
    listen_fd = listen_fd < 0 ? -1 : listen_fd;

    switch (listen_fd) {
        case -1: {
            if (!host_name) {
                return RpcStack(-1);
            } else {
                int len = offsetof(struct sockaddr_un, sun_path) + strlen(host_name);
                struct sockaddr_un un;

                memset(&un, 0, sizeof(un));
                unlink(host_name);

                listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
                un.sun_family = AF_UNIX;

                if (anonymity) {
                    strncpy(un.sun_path + 1, host_name, sizeof(un.sun_path) - 2);
                    len += 1;
                } else {
                    strncpy(un.sun_path, host_name, sizeof(un.sun_path) - 1);
                }

                if (listen_fd >= 0) {
                    if (bind(listen_fd, (struct sockaddr*)&un, len) < 0 || listen(listen_fd, 5) < 0) {
                        RPC_DBG("listen failed, errno" << errno << ":" << strerror(errno));
                        close(listen_fd);
                        listen_fd = -1;
                        return RpcStack(-1);
                    }
                }
            }
        }
        default: {
            struct timeval interval = { ms_timeout / 1000, (ms_timeout % 1000) * 1000 };
            fd_set set;
            int accept_fd = -1;
            int flags = fcntl(listen_fd, F_GETFL, 0);

            if (listen_fd >= 0) {
                if (fcntl(listen_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
                    RPC_DBG("fcntl listen socket NonBlock failed, errno "
                        << errno << ":" << strerror(errno));
                }

                FD_ZERO(&set);
                FD_SET(listen_fd, &set);

                if ((select(listen_fd + 1, &set, nullptr, nullptr,
                        ms_timeout >= 0 ? &interval : nullptr))
                    > 0) {
                    struct sockaddr_un un;
                    socklen_t len = sizeof(un);
                    accept_fd = accept(listen_fd, (struct sockaddr*)&un, &len);
                }

                if (fcntl(listen_fd, F_SETFL, flags & ~O_NONBLOCK) < 0) {
                    RPC_DBG("fcntl listen socket Block failed, errno "
                        << errno << ":" << strerror(errno));
                }

                if (accept_fd < 0) {
                    RPC_DBG("accept socket failed, errno " << errno << ":"
                                                        << strerror(errno));
                }
            }
        }
    }

    return RpcStack(-1);
}

/**
 * @brief Caller Decorator
 *
 * @tparam RET Remote function ret type
 */

template <class RET>
class RpcCallProxy {
private:
    RpcStack& stack_;

    template <class... __ARGS>
    bool pushStack(__ARGS... args) {
        return true;
    }

    template <class __THIS_ARG, class... __ARGS>
    bool pushStack(const __THIS_ARG* first_arg, __ARGS... args) {
        bool ret = pushStack(args...);

        /* Push Current Argument */
        ret &= stack_.pushStack(first_arg, sizeof(__THIS_ARG));
        return ret;
    }

    template <class __THIS_ARG, class... __ARGS>
    bool pushStack(const __THIS_ARG& first_arg, __ARGS... args) {
        bool ret = pushStack(args...);

        /* Push Current Argument */
        ret &= stack_.pushStack(&first_arg, sizeof(__THIS_ARG));
        return ret;
    }

public:
    template <class T>
    RpcCallProxy(RpcStack& stack, T symbol):stack_(stack) {
        /**
         * Open function ,push arguments and get ret value like a local function
         */
        if (!stack_.pushStack(&symbol, sizeof(T))) {
            throw k_rpc_func_call_failed;
        }
    }

    template <class T>
    RpcCallProxy(RpcStack& stack, const T* symbol):stack_(stack) {
        /**
         * Open function ,push arguments and get ret value like a local function
         */
        if (!stack_.pushStack(symbol, sizeof(T))) {
            throw k_rpc_func_call_failed;
        }
    }
    ~RpcCallProxy() { }

    template <class... __ARGS>
    RET operator()(const __ARGS... args) {
        RET ret;

        if (!pushStack(args...)) {
            throw k_rpc_param_push_failed;
        }

        if (!stack_.popReturn(&ret, sizeof(RET))) {
            throw k_rpc_ret_get_failed;
        }

        return ret;
    }
};

/**
 * @brief specialization for function call without return value
 *
 * @tparam
 * @tparam __ARGS
 * @param args
 */
template <>
template <class... __ARGS>
void RpcCallProxy<void>::operator()(const __ARGS... args) {
    if (!pushStack(args...)) {
        throw k_rpc_param_push_failed;
    }

    return;
}

/**
 * @brief Args Stack for Rpc Server
 */
template <class... __ARGS_TYPE>
class RpcArgs;

template <>
class RpcArgs<> {
private:
    RpcStack& stack_;

public:
    RpcArgs(RpcStack& stack):stack_(stack) { }
    virtual ~RpcArgs() { }
};

template <class __THIS_TYPE, class... __ARGS_TYPE>
class RpcArgs<__THIS_TYPE, __ARGS_TYPE...> : public RpcArgs<__ARGS_TYPE...> {
private:
    RpcStack& stack_;
    using __VALUE_TYPE = typename std::remove_reference<__THIS_TYPE>::type;
    __VALUE_TYPE value_; // Server args stack consists of per inherit RpcArgs class
public:
    RpcArgs(RpcStack& stack):RpcArgs<__ARGS_TYPE...>(stack)
                           , stack_(stack)
    {
        if (!stack_.popStack(&value_, sizeof(__VALUE_TYPE))) {
            // System call maybe broken by client disconnect, stop Creating function
            throw k_rpc_callfun_interrupt;
        }
    }
    virtual ~RpcArgs() { }

    __VALUE_TYPE& getValue() { return value_; }
};

/**
 * @brief Get Nth value in arguments stack
 *
 * @tparam N index of arguments
 * @tparam __ARGS_TYPE arguments types
 */
template <int N, class... __ARGS_TYPE>
struct element;

// if N > size assert error
template <int N>
struct element<N, RpcArgs<>> {
    static_assert(0 > N, "Index outside of argument stack!");
};

// find finnal
template <class __THIS_TYPE, class... __ARGS_TYPE>
struct element<0, RpcArgs<__THIS_TYPE, __ARGS_TYPE...>> {
    using value_t = __THIS_TYPE;
    using class_t = RpcArgs<__THIS_TYPE, __ARGS_TYPE...>;
};

// Expand arguments
template <int N, class __THIS_TYPE, class... __ARGS_TYPE>
struct element<N, RpcArgs<__THIS_TYPE, __ARGS_TYPE...>>
    : public element<N - 1, RpcArgs<__ARGS_TYPE...>> { };

// Get argument with index of N
template <int N, class... __ARGS_TYPE>
auto getArgs(RpcArgs<__ARGS_TYPE...>& args) {
    using __class_t = typename element<N, RpcArgs<__ARGS_TYPE...>>::class_t;
    return ((__class_t&)args).getValue();
}

template <int N, class... __ARGS_TYPE>
auto getArgs(RpcArgs<__ARGS_TYPE...>&& args) {
    using __class_t = typename element<N, RpcArgs<__ARGS_TYPE...>>::class_t;
    return ((__class_t&&)args).getValue();
}

/**
 * @brief Get Arguments nums
 */
template <class T>
struct args_size;

template <class... __ARGS>
struct args_size<RpcArgs<__ARGS...>> {
    static constexpr const unsigned int value = sizeof...(__ARGS);
};

#if 0

/* traits a class object'soperator() function return type  */
template <typename C>
struct result_traits : result_traits<decltype(&C::operator())> {};

/* traits a point object's return type which type is function */
template <typename T>
struct result_traits<T*> : result_traits<T> {};

/* traits normal function return type */
template <typename R, typename... P>
struct result_traits<R(*)(P...)>{ typedef R type; };

/* traits member function return type */
template <typename R, typename C, typename... P> 
struct result_traits<R(C::*)(P...)> { typedef R type; };

/* traits const member function return type */
template <typename R, typename C, typename... P> 
struct result_traits<R(C::*)(P...) const> { typedef R type; };

#endif

/**
 * @brief Base Rpc Bind Class for Polumorphism in run time
 *
 */
class RpcServerProxyBase {
public:
    RpcServerProxyBase() { }
    virtual ~RpcServerProxyBase() { }
    virtual rpc_err_t operator()(RpcStack& stack) = 0;
};

/**
 * @brief Server Bind function
 *
 * @tparam __RET_TYPE function ret type
 * @tparam __ARGS_TYPE function arguments type list
 */
template <class __FUN_TYPE, class __RET_TYPE, class... __ARGS_TYPE>
class RpcServerProxy : public RpcServerProxyBase {
private:
    // 绑定的函数类型和返回值类型
    using func_t = typename std::decay<__FUN_TYPE>::type;
    using ret_t = __RET_TYPE;

    // 绑定函数
    func_t& proxy_func_;

private:
    template <class __ARGS, std::size_t... I>
    ret_t do_call(__ARGS& args, std::index_sequence<I...>) {
        return proxy_func_(getArgs<I>(args)...);
    }

    ret_t do_call(RpcArgs<__ARGS_TYPE...>& args) {
        return do_call(args, std::make_index_sequence<args_size<RpcArgs<__ARGS_TYPE...>>::value>());
    }

public:
    RpcServerProxy(func_t& bind_func):proxy_func_(bind_func) { };
    RpcServerProxy(func_t&& bind_func):proxy_func_(bind_func) { };

    virtual ~RpcServerProxy() { };

    rpc_err_t operator()(RpcStack& stack) {
        try {
            /**
             * Receive Args in RpcArgs
             * and input to process for bind funciton
             * return value
             */
            RpcArgs<__ARGS_TYPE...> Args(stack);

            const ret_t& ret = do_call(Args);
            stack.pushReturn(&ret, sizeof(ret_t));
        } catch (rpc_err_t err) {
            return err;
        }

        return k_rpc_ok;
    }
};

/**
 * @brief Server Bind function without return value
 *
 * @tparam __RET_TYPE function ret type
 * @tparam __ARGS_TYPE function arguments type list
 */
template <class __FUN_TYPE, class... __ARGS_TYPE>
class RpcServerProxy<__FUN_TYPE, void, __ARGS_TYPE...>:public RpcServerProxyBase {
private:
    // 绑定的函数类型和返回值类型
    using func_t = typename std::decay<__FUN_TYPE>::type;

    // 绑定函数
    func_t& proxy_func_;

private:
    template <class __ARGS, std::size_t... I>
    void do_call(__ARGS& args, std::index_sequence<I...>) {
        proxy_func_(getArgs<I>(args)...);
    }

    void do_call(RpcArgs<__ARGS_TYPE...>& args) {
        do_call(args, std::make_index_sequence<args_size<RpcArgs<__ARGS_TYPE...>>::value>());
    }

public:
    RpcServerProxy(func_t& bind_func):proxy_func_(bind_func) { };
    RpcServerProxy(func_t&& bind_func):proxy_func_(bind_func) { };

    virtual ~RpcServerProxy() { };

    rpc_err_t operator()(RpcStack& stack) {
        try {
            /**
             * Receive Args in RpcArgs
             * and input to process for bind funciton
             * return value
             */
            RpcArgs<__ARGS_TYPE...> Args(stack);

            do_call(Args);
        } catch (rpc_err_t err) {
            return err;
        }

        return k_rpc_ok;
    }
};

namespace detail {
    template <typename R, typename... Args>
    struct __function_traits_base {
        using function_type = R (*)(Args...);
        using result_type = R;
        using argument_types = std::tuple<Args...>;
    };

    template <typename F>
    struct __function_traits;
    template <typename F>
    struct __function_traits<std::reference_wrapper<F>>
        : public __function_traits<F> { };
    template <typename R, typename... Args>
    struct __function_traits<R (*)(Args...)>
        : public __function_traits_base<R, Args...> { };
    template <typename R, typename C, typename... Args>
    struct __function_traits<R (C::*)(Args...)>
        : public __function_traits_base<R, Args...> { };
    template <typename R, typename C, typename... Args>
    struct __function_traits<R (C::*)(Args...) const>
        : public __function_traits_base<R, Args...> { };
    template <typename F>
    struct __function_traits : public __function_traits<decltype(&F::operator())> {
    };
} // namespace detail

template <typename F>
struct function_traits : public detail::__function_traits<std::decay_t<F>> { };

/* check lamda function */
template <typename R, typename C, typename... __ARGS_TYPE>
auto bind(C& lambda, R (C::*useless)(__ARGS_TYPE...) const) -> RpcServerProxy<decltype(lambda), R, __ARGS_TYPE...> {
    return RpcServerProxy<decltype(lambda), R, __ARGS_TYPE...>(
        std::forward<C>(lambda));
}

template <typename R, typename C, typename... __ARGS_TYPE>
auto bind(C& lambda, R (C::*useless)(__ARGS_TYPE...)) -> RpcServerProxy<decltype(lambda), R, __ARGS_TYPE...> {
    return RpcServerProxy<decltype(lambda), R, __ARGS_TYPE...>(
        std::forward<C>(lambda));
}

template <typename R, typename C, typename... __ARGS_TYPE>
auto bind(C&& lambda, R (C::*useless)(__ARGS_TYPE...) const) -> RpcServerProxy<decltype(lambda), R, __ARGS_TYPE...> {
    return RpcServerProxy<decltype(lambda), R, __ARGS_TYPE...>(
        std::forward<C>(lambda));
}

template <typename R, typename C, typename... __ARGS_TYPE>
auto bind(C&& lambda, R (C::*useless)(__ARGS_TYPE...)) -> RpcServerProxy<decltype(lambda), R, __ARGS_TYPE...> {
    return RpcServerProxy<decltype(lambda), R, __ARGS_TYPE...>(
        std::forward<C>(lambda));
}

template <typename C>
auto bind(C& lambda) {
    return bind(std::forward<C>(lambda), decltype (&C::operator())());
}

template <typename C>
auto bind(C&& lambda) {
    return bind(std::forward<C>(lambda), decltype (&C::operator())());
}

/* check function */
template <typename R, typename... __ARGS_TYPE>
auto bind(R (*func)(__ARGS_TYPE...)) -> RpcServerProxy<decltype(func), R, __ARGS_TYPE...> {
    return RpcServerProxy<decltype(func), R, __ARGS_TYPE...>(func);
}

} // namespace ez_rpc

#endif
